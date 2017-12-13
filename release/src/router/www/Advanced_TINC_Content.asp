<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - TINC</title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<script language="JavaScript" type="text/javascript" src="/state.js"></script>
<script language="JavaScript" type="text/javascript" src="/general.js"></script>
<script language="JavaScript" type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" language="JavaScript" src="/help.js"></script>
<script type="text/javascript" language="JavaScript" src="/validator.js"></script>
<script>
function applyRule(){
	if(validForm()){
		showLoading();
		document.form.submit();
	}
}

function validForm(){
	return true;
}

function done_validating(action){
	refreshpage();
}

function initial(){
	show_menu();
	tinc_enable_check();
}

function tinc_enable_check(){
	var tinc_enable = '<% nvram_get("tinc_enable"); %>';
	if(tinc_enable == 1){
		document.form.tinc_enable[0].checked = "true";
		document.getElementById('tinc_url_tr').style.display = "";
		document.getElementById('tinc_url_tr').style.display = "";
		document.getElementById('tinc_url_tr').style.display = "";
	}	
	else{
		document.form.tinc_enable[1].checked = "true";
		document.getElementById('tinc_url_tr').style.display = "none";
		document.getElementById('tinc_id_tr').style.display = "none";
		document.getElementById('tinc_passwd_tr').style.display = "none";
	}
}

function tinc_on_off(){
	if(document.form.tinc_enable[0].checked) {
		document.getElementById('tinc_url_tr').style.display = "";
		document.getElementById('tinc_id_tr').style.display = "";
		document.getElementById('tinc_passwd_tr').style.display = "";
	}
	else{
		document.form.tinc_enable[1].checked = "true";
		document.getElementById('tinc_url_tr').style.display = "none";
		document.getElementById('tinc_id_tr').style.display = "none";
		document.getElementById('tinc_passwd_tr').style.display = "none";
	}
}
</script>
</head>

<body onload="initial();">
<div id="TopBanner"></div>
<div id="Loading" class="popup_bg"></div>

<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

<form method="post" name="form" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="Advanced_TINC_Content.asp">
<input type="hidden" name="next_page" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_wait" value="5">
<input type="hidden" name="action_script" value="restart_wan_if">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">

<table class="content" align="center" cellpadding="0" cellspacing="0">
	<tr>
		<td width="17">&nbsp;</td>		
		<td valign="top" width="202">				
			<div id="mainMenu"></div>	
			<div id="subMenu"></div>		
		</td>						
		<td valign="top">
			<div id="tabMenu" class="submenuBlock"></div>
		<!--===================================Beginning of Main Content===========================================-->		
			<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
				<tr>
					<td valign="top" >	
						<table width="760px" border="0" cellpadding="4" cellspacing="0" class="FormTitle" id="FormTitle">
							<tbody>
							<tr>
								<td bgcolor="#4D595D" valign="top" >
									<div>&nbsp;</div>
									<div class="formfonttitle">
										VPN - TINC客户端
									</div>
									<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img src="/images/New_ui/export/line_export.png"></div>
									<div class="formfontdesc">
										TINC客户端:
										<ul>
											<li>不要与其他VPN同时开启。</li>
											<li>设备ID是唯一的，同一ID同时只允许一台设备使用。</li>
											<li>应用本页面设置后会重启广域网。</li>
										</ul>
									</div>
									<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<tr>
											<th>开启TINC客户端</th>
											<td>
												<input type="radio" value="1" name="tinc_enable" class="input" onclick="tinc_on_off()" <% nvram_match("tinc_enable", "1", "checked"); %>>是
												<input type="radio" value="0" name="tinc_enable" class="input" onclick="tinc_on_off()" <% nvram_match("tinc_enable", "0", "checked"); %>>否
											</td>
										</tr>
										<tr id="tinc_url_tr">
											<th>远程配置地址</th>
											<td>
												<input type="text" maxlength="128" class="input_32_table" name="tinc_url" value="<% nvram_get("tinc_url"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										<tr id="tinc_id_tr">
											<th>设备ID</th>
											<td>
												<input type="text" maxlength="16" class="input_15_table" name="tinc_id" value="<% nvram_get("tinc_id"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										<tr id="tinc_passwd_tr">
											<th>密码</th>
											<td>
												<input type="text" maxlength="16" class="input_15_table" name="tinc_passwd" value="<% nvram_get("tinc_passwd"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										</table>
									<div class="apply_gen">
										<input name="button" type="button" class="button_gen" onclick="applyRule()" value="<#CTL_apply#>"/>
									</div>
								</td>
							</tr>
							</tbody>	
						</table>
					</td>
</form>
				</tr>
			</table>				
		<!--===================================Ending of Main Content===========================================-->		
		</td>		
		<td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>

<div id="footer"></div>
</body>
</html>
