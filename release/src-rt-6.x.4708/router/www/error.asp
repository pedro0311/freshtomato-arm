<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Error</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<style type="text/css">
#div {
	width:600px;
	height:40px;
	color:rgb(200,200,200);
	background-color:rgb(22,22,22);
	position:absolute;
	top:0;
	bottom:0;
	left:0;
	right:0;
	font:14px Tahoma,Arial,sans-serif;
	text-align:center;
	margin:auto;
	padding:10px 10px;
	border-radius:5px;
}
input {
	width:80px;
	height:24px;
	cursor:pointer;
}
</style>
</head>
<body>
	<div id="div">
		<form action="">
			<div style="padding:10px 0">
				<script type="text/javascript">
//	<% resmsg('Unknown error'); %>
				document.write(resmsg);
				</script>&nbsp;
				<input type="button" value="Back" onclick="history.go(-1)">
			</div>
		</form>
	</div>
</body>
</html>