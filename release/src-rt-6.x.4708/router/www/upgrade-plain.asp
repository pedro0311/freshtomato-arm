<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>Firmware Upgrade</title>
<style type="text/css">
body {
    background:rgb(0,0,0) url(tomatousb_bg.png);
    font:14px Tahoma,Arial,sans-serif;
    color:rgb(255,255,255);
}
input {
    width:80px;
    height:24px;
}
.div {
    width:800px;
    height:240px;
    background-color:rgb(47,61,64);
    position:absolute;
    top:0;
    bottom:0;
    left:0;
    right:0;
    text-align:center;
    margin:auto;
    padding:10px 10px;
    border-radius:5px;
}
</style>
</head>
<body>
    <div class="div">
		<h1>Firmware Upgrade</h1>
		<b>WARNING:</b>
		<ul>
			<li>There is no upload status information in this page and there will be no
				change in the display after the Upgrade button is pushed. You will be shown a
				new page only after the upgrade completes.
			<li>It may take up to 3 minutes for the upgrade to complete. Do not interrupt
				the router or the browser during this time.
		</ul>
		<br />
		<form name="firmware_upgrade" method="post" action="upgrade.cgi?<% nv(http_id) %>" encType="multipart/form-data">
			<div>
				<input type="hidden" name="submit_button" value="Upgrade">
				Firmware: <input type="file" name="file"> <input type="submit" value="Upgrade">
			</div>
		</form>
	</div>
</body>
</html>