<!DOCTYPE html>
<html lang="en-GB">
<head>
<title>Firmware Upgrade</title>
<link rel="stylesheet" type="text/css" href="/tomato.css">
<% css(); %>
<style>
div.tomato-grid.container-div {
	width: 770px;
	height: 210px;
}
</style>
</head>

<body>
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<form name="firmware_upgrade" method="post" action="upgrade.cgi?<% nv(http_id) %>" enctype="multipart/form-data">
				<div class="info-centered">
					<div style="font-size:20px">Firmware Upgrade</div><br>
					<b>WARNING:</b><br>
					There is no upload status information in this page and there will be no change in the display after the Upgrade button is pushed. You will be shown a new page only after the upgrade completes.<br>
					It may take up to 3 minutes for the upgrade to complete. Do not interrupt the router or the browser during this time.<br>
					<br><br>
					Firmware: <input type="file" name="file"> <input type="submit" value="Upgrade"><input type="hidden" name="submit_button" value="Upgrade">
				</div>
			</form>
		</div>
	</div>
</div>
</body>
</html>
