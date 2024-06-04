#!/usr/bin/env python3

# (C) 2021 by Arturo Borrero Gonzalez <arturo@netfilter.org>

#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#

# tests.yaml file format:
#  - name: "test 1"
#    scenario: scenario1
#    test:
#      - test1 cmd1
#      - test1 cmd2

# scenarios.yaml file format:
# - name: scenario1
#   start:
#     - cmd1
#     - cmd2
#   stop:
#     - cmd1
#     - cmd2

# env.yaml file format:
# - VAR1: value1
# - VAR2: value2

import os
import sys
import argparse
import subprocess
import yaml
import logging


def read_yaml_file(file):
    try:
        with open(file, "r") as stream:
            try:
                return yaml.safe_load(stream)
            except yaml.YAMLError as e:
                logging.error(e)
                exit(2)
    except FileNotFoundError as e:
        logging.error(e)
        exit(2)


def validate_dictionary(dictionary, keys):
    if not isinstance(dictionary, dict):
        logging.error("not a dictionary:\n{}".format(dictionary))
        return False
    for key in keys:
        if dictionary.get(key) is None:
            logging.error("missing key {} in dictionary:\n{}".format(key, dictionary))
            return False
    return True


def stage_validate_config(args):
    scenarios_dict = read_yaml_file(args.scenarios_file)
    for definition in scenarios_dict:
        if not validate_dictionary(definition, ["name", "start", "stop"]):
            logging.error("couldn't validate file {}".format(args.scenarios_file))
            return False

    logging.debug("{} seems valid".format(args.scenarios_file))
    ctx.scenarios_dict = scenarios_dict

    tests_dict = read_yaml_file(args.tests_file)
    for definition in tests_dict:
        if not validate_dictionary(definition, ["name", "scenario", "test"]):
            logging.error("couldn't validate file {}".format(args.tests_file))
            return False

    logging.debug("{} seems valid".format(args.tests_file))
    ctx.tests_dict = tests_dict

    env_list = read_yaml_file(args.env_file)
    if not isinstance(env_list, list):
        logging.error("couldn't validate file {}".format(args.env_file))
        return False

    # set env to default values if not overridden when calling this very script
    for entry in env_list:
        for key in entry:
            os.environ[key] = os.getenv(key, entry[key])


def cmd_run(cmd):
    logging.debug("running command: {}".format(cmd))
    r = subprocess.run(cmd, shell=True)
    if r.returncode != 0:
        logging.warning("failed command: {}".format(cmd))
    return r.returncode


def scenario_get(name):
    for n in ctx.scenarios_dict:
        if n["name"] == name:
            return n

    logging.error("couldn't find a definition for scenario '{}'".format(name))
    exit(1)


def scenario_start(scenario):
    for cmd in scenario["start"]:
        if cmd_run(cmd) == 0:
            continue

        logging.warning("--- failed scenario: {}".format(scenario["name"]))
        ctx.counter_scenario_failed += 1
        ctx.skip_current_test = True
        return


def scenario_stop(scenario):
    for cmd in scenario["stop"]:
        cmd_run(cmd)


def test_get(name):
    for n in ctx.tests_dict:
        if n["name"] == name:
            return n

    logging.error("couldn't find a definition for test '{}'".format(name))
    exit(1)


def _test_run(test_definition):
    if ctx.skip_current_test:
        return

    for cmd in test_definition["test"]:
        if cmd_run(cmd) == 0:
            continue

        logging.warning("--- failed test: {}".format(test_definition["name"]))
        ctx.counter_test_failed += 1
        return

    logging.info("--- passed test: {}".format(test_definition["name"]))
    ctx.counter_test_ok += 1


def test_run(test_definition):
    scenario = scenario_get(test_definition["scenario"])

    logging.info("--- running test: {}".format(test_definition["name"]))

    scenario_start(scenario)
    _test_run(test_definition)
    scenario_stop(scenario)


def stage_run_tests(args):
    if args.start_scenario:
        scenario_start(scenario_get(args.start_scenario))
        return

    if args.stop_scenario:
        scenario_stop(scenario_get(args.stop_scenario))
        return

    if args.single:
        test_run(test_get(args.single))
        return

    for test_definition in ctx.tests_dict:
        ctx.skip_current_test = False
        test_run(test_definition)


def stage_report():
    logging.info("---")
    logging.info("--- finished")
    total = ctx.counter_test_ok + ctx.counter_test_failed + ctx.counter_scenario_failed
    logging.info("--- passed tests: {}".format(ctx.counter_test_ok))
    logging.info("--- failed tests: {}".format(ctx.counter_test_failed))
    logging.info("--- scenario failure: {}".format(ctx.counter_scenario_failed))
    logging.info("--- total tests: {}".format(total))


def parse_args():
    description = "Utility to run tests for conntrack-tools"
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "--tests-file",
        default="tests.yaml",
        help="File with testcase definitions. Defaults to '%(default)s'",
    )
    parser.add_argument(
        "--scenarios-file",
        default="scenarios.yaml",
        help="File with configuration scenarios for tests. Defaults to '%(default)s'",
    )
    parser.add_argument(
        "--env-file",
        default="env.yaml",
        help="File with environment variables for scenarios/tests. Defaults to '%(default)s'",
    )
    parser.add_argument(
        "--single",
        help="Execute a single testcase and exit. Use this for developing testcases",
    )
    parser.add_argument(
        "--start-scenario",
        help="Execute scenario start commands and exit. Use this for developing testcases",
    )
    parser.add_argument(
        "--stop-scenario",
        help="Execute scenario stop commands and exit. Use this for cleanup",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="debug mode",
    )

    return parser.parse_args()


class Context:
    def __init__(self):
        self.scenarios_dict = None
        self.tests_dict = None
        self.counter_test_failed = 0
        self.counter_test_ok = 0
        self.counter_scenario_failed = 0
        self.skip_current_test = False


# global data
ctx = Context()


def main():
    args = parse_args()

    logging_format = "[%(filename)s] %(levelname)s: %(message)s"
    if args.debug:
        logging_level = logging.DEBUG
    else:
        logging_level = logging.INFO
    logging.basicConfig(format=logging_format, level=logging_level, stream=sys.stdout)

    if os.geteuid() != 0:
        logging.error("root required")
        exit(1)

    stage_validate_config(args)
    stage_run_tests(args)
    stage_report()


if __name__ == "__main__":
    main()
