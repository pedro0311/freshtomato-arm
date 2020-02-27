<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>Firmware Upgrade</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<style type="text/css">
#div {
	width:770px;
	height:190px;
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
</style>
</head>
<body>
	<div id="div">
		<h2>Firmware Upgrade</h2>
		<b>WARNING:</b><br/>
		There is no upload status information in this page and there will be no change in the display after the Upgrade button is pushed. You will be shown a new page only after the upgrade completes.<br/>
		It may take up to 3 minutes for the upgrade to complete. Do not interrupt the router or the browser during this time.
		<br/><br/>
		<form name="firmware_upgrade" method="post" action="upgrade.cgi?<% nv(http_id) %>" enctype="multipart/form-data">
			<div>
				<input type="hidden" name="submit_button" value="Upgrade">
				Firmware: <input type="file" name="file"> <input type="submit" value="Upgrade">
			</div>
		</form>
	</div>
</body>
</html>