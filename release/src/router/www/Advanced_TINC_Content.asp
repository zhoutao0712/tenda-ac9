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
<title><#Web_Title#> - CG加速</title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<script language="JavaScript" type="text/javascript" src="/state.js"></script>
<script language="JavaScript" type="text/javascript" src="/general.js"></script>
<script language="JavaScript" type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" language="JavaScript" src="/help.js"></script>
<script type="text/javascript" language="JavaScript" src="/validator.js"></script>
<script type="text/javascript" language="JavaScript" src="/js/jquery.js"></script>
<script>
var tinc_rulelist_array = [];
var add_ruleList_array = new Array();

function validForm(){
	return true;
}

function done_validating(action){
	refreshpage();
}

function initial(){
	show_menu();
	tinc_enable_check();

	//parse nvram to array
	var parseNvramToArray = function(oriNvram) {
		var parseArray = [];
		var oriNvramRow = decodeURIComponent(oriNvram).split('<');

		for(var i = 0; i < oriNvramRow.length; i += 1) {
			if(oriNvramRow[i] != "") {
				var oriNvramCol = oriNvramRow[i].split('>');
				var eachRuleArray = new Array();
				var action = oriNvramCol[0];
				eachRuleArray.push(action);
				var host = oriNvramCol[1];
				eachRuleArray.push(host);
				parseArray.push(eachRuleArray);
			}
		}
		return parseArray;
	};
	tinc_rulelist_array["tinc_rulelist_0"] = parseNvramToArray('<% nvram_char_to_ascii("","tinc_rulelist"); %>');
	Object.keys(tinc_rulelist_array).forEach(function(key) {
		gen_tinc_ruleTable_Block(key);
	});
//	gen_tinc_ruleTable_Block("tinc_rulelist_0");

	Object.keys(tinc_rulelist_array).forEach(function(key) {
		showtinc_rulelist(tinc_rulelist_array[key], key);
	});
//	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_0"], "tinc_rulelist_0");

}

function tinc_enable_check(){
	var tinc_enable = '<% nvram_get("tinc_enable"); %>';
	if(tinc_enable == 1){
		document.form.tinc_enable[0].checked = "true";
//		document.getElementById('tinc_url_tr').style.display = "";
		document.getElementById('tinc_id_tr').style.display = "";
//		document.getElementById('tinc_passwd_tr').style.display = "";
		document.getElementById('tinc_guest_tr').style.display = "";
	}	
	else{
		document.form.tinc_enable[1].checked = "true";
//		document.getElementById('tinc_url_tr').style.display = "none";
		document.getElementById('tinc_id_tr').style.display = "none";
//		document.getElementById('tinc_passwd_tr').style.display = "none";
		document.getElementById('tinc_guest_tr').style.display = "none";
	}
}

function tinc_on_off(){
	if(document.form.tinc_enable[0].checked) {
		document.form.tinc_enable[0].checked = "true";
//		document.getElementById('tinc_url_tr').style.display = "";
		document.getElementById('tinc_id_tr').style.display = "";
//		document.getElementById('tinc_passwd_tr').style.display = "";
		document.getElementById('tinc_guest_tr').style.display = "";
	}
	else{
		document.form.tinc_enable[1].checked = "true";
//		document.getElementById('tinc_url_tr').style.display = "none";
		document.getElementById('tinc_id_tr').style.display = "none";
//		document.getElementById('tinc_passwd_tr').style.display = "none";
		document.getElementById('tinc_guest_tr').style.display = "none";
	}
}

function validRuleForm(_wan_idx){	
//	alert(document.getElementById("tinc_host_x_" + _wan_idx + "").value);
	add_ruleList_array = [];
	add_ruleList_array.push(document.getElementById("tinc_action_x_" + _wan_idx + "").value);
	add_ruleList_array.push(document.getElementById("tinc_host_x_" + _wan_idx + "").value);
	return true;
}

function addRow_Group(upper, _this){
	var wan_idx = $(_this).closest("*[wanUnitID]").attr( "wanUnitID" );
	if(validRuleForm(wan_idx)){

		var rule_num = tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""].length;
		if(rule_num >= upper){
			alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
			return false;
		}

		//match(Source Target + Port Range + Protocol) is not accepted
		var tinc_rulelist_array_temp = tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""].slice();
		var add_ruleList_array_temp = add_ruleList_array.slice();
		if(tinc_rulelist_array_temp.length > 0) {
			tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""].push(add_ruleList_array);
		}
		else {
			tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""].push(add_ruleList_array);
		}

		document.getElementById("tinc_action_x_" + wan_idx + "").value = "+";
		document.getElementById("tinc_host_x_" + wan_idx + "").value = "";
		showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""], "tinc_rulelist_" + wan_idx + "");
		return true;
	}
}

function del_Row(_this){
	var row_idx = $(_this).closest("*[row_tr_idx]").attr( "row_tr_idx" );
	var wan_idx = $(_this).closest("*[wanUnitID]").attr( "wanUnitID" );
	
	tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""].splice(row_idx, 1);
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_" + wan_idx + ""], "tinc_rulelist_" + wan_idx + "");
}

function showtinc_rulelist(_arrayData, _tableID) {
	var wan_idx = _tableID.split("_")[2];
	var width_array = [14, 75, 11];
	var handle_long_text = function(_len, _text, _width) {
		var html = "";
		if(_text.length > _len) {
			var display_text = "";
			display_text = _text.substring(0, (_len - 2)) + "...";
			html +='<td width="' +_width + '%" title="' + _text + '">' + display_text + '</td>';
		}
		else
			html +='<td width="' + _width + '%">' + _text + '</td>';
		return html;
	};
	var code = "";
	code +='<table width="100%" cellspacing="0" cellpadding="4" align="center" class="list_table">';
	if(_arrayData.length == 0)
		code +='<tr><td style="color:#FFCC00;" colspan="7"><#IPConnection_VSList_Norule#></td></tr>';
	else {
		for(var i = 0; i < _arrayData.length; i += 1) {
			var eachValue = _arrayData[i];
			if(eachValue.length != 0) {
				code +='<tr row_tr_idx="' + i + '">';
				for(var j = 0; j < eachValue.length; j += 1) {
					switch(j) {
						case 0 :
						case 1 :
							code += handle_long_text(18, eachValue[j], width_array[j]);
							break;
						case 2 :
							code += handle_long_text(14, eachValue[j], width_array[j]);
							break;
						default :
							code +='<td width="' + width_array[j] + '%">' + eachValue[j] + '</td>';
							break;
					}
				}
				
				code +='<td width="14%"><input class="remove_btn" onclick="del_Row(this);" value=""/></td></tr>';
				code +='</tr>';
			}
		}
	}
	code +='</table>';
	document.getElementById("tinc_rulelist_Block_" + wan_idx + "").innerHTML = code;	     
}

function gen_tinc_ruleTable_Block(_tableID) {
	var html = "";
	html += '<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable_table">';
	
	html += '<thead>';
	html += '<tr>';
	var wan_title = "";
	var wan_idx = _tableID.split("_")[2];
/*
	if(support_dual_wan_unit_flag) {
		switch(wan_idx) {
			case "0" :
				wan_title = "<#dualwan_primary#>&nbsp;";
				break;
			case "1" :
				wan_title = "<#dualwan_secondary#>&nbsp;";
				break;
		}
	}
*/
	html += '<td colspan="7">' + wan_title + '域名规则&nbsp;(<#List_limit#>&nbsp;64)</td>';
	html += '</tr>';
	html += '</thead>';

	html += '<tr>';
	html += '<th width="14%">动作</th>';

	html += '<th width="75%"><a class="hintstyle" href="javascript:void(0);" onClick="">域名</a></th>';
	html += '<th width="11%"><#list_add_delete#></th>';
	html += '</tr>';

	html += '<tr>';

	html += '<td width="14%">';
	html += '<select name="tinc_action_x_' + wan_idx + '" id="tinc_action_x_' + wan_idx + '" class="input_option">';
	html += '<option value="+">增加到列表</option>';
	html += '<option value="-">从列表删除</option>';
	html += '</select>';
	html += '</td>';

	html += '<td width="75%">';
	html += '<input type="text" maxlength="128" class="input_32_table" name="tinc_host_x_' + wan_idx + '" id="tinc_host_x_' + wan_idx + '" onKeyPress="return validator.isString(this, event)" autocorrect="off" autocapitalize="off"/>';
	html += '</td>';

	html += '<td width="11%">';
	html += '<input type="button" class="add_btn" onClick="addRow_Group(64, this);" name="tinc_rulelist_' + wan_idx + '" id="tinc_rulelist_' + wan_idx + '" value="">';
	html += '</td>';

	html += '</tr>';
	html += '</table>';

	document.getElementById("tinc_rulelist_Table_" + wan_idx + "").innerHTML = html;
}

function applyRule(){
	if(validForm()){
		//parse array to nvram
		var parseArrayToNvram = function(_dataArray) {
			var tmp_value = "";
			for(var i = 0; i < _dataArray.length; i += 1) {
				if(_dataArray[i].length != 0) {
					tmp_value += "<";
					var action = _dataArray[i][0];
					tmp_value += action + ">";
					var host = _dataArray[i][1];
					tmp_value += host;
				}
			}
			return tmp_value;
		};

		document.form.tinc_rulelist.value = parseArrayToNvram(tinc_rulelist_array["tinc_rulelist_0"]);
//		alert(document.form.tinc_rulelist.value);

		showLoading();
		document.form.submit();
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
<input type="hidden" name="tinc_rulelist" value=''>

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
										VPN - CG加速
									</div>
									<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img src="/images/New_ui/export/line_export.png"></div>
									<div class="formfontdesc">
										<ul>
											<li>不要与其他VPN同时开启。</li>
											<li>设备ID是唯一的，同一ID同时只允许一台设备使用。</li>
											<li>应用本页面设置后会重启广域网。</li>
											<li>访客网络全局模式仅仅针对索引为1的访客SSID有效。自行到访客网络去开启就生效了。</li>
										</ul>
									</div>
									<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<thead>
											<tr>
												<td colspan="4">基本设置</td>
											</tr>
										</thead>
										<tr>
											<th>开启</th>
											<td>
												<input type="radio" value="1" name="tinc_enable" class="input" onclick="tinc_on_off()" <% nvram_match("tinc_enable", "1", "checked"); %>>是
												<input type="radio" value="0" name="tinc_enable" class="input" onclick="tinc_on_off()" <% nvram_match("tinc_enable", "0", "checked"); %>>否
											</td>
										</tr>
										<tr id="tinc_guest_tr">
											<th>访客网络全局模式</th>
											<td>
												<select name="tinc_guest_enable" class="input_option">
													<option class="content_input_fd" value="0" <% nvram_match("tinc_guest_enable", "0","selected"); %>>停用</option>
													<option class="content_input_fd" value="1" <% nvram_match("tinc_guest_enable", "1","selected"); %>>开启</option>
												</select>
<!--
												<input type="radio" value="1" name="tinc_enable" class="input" <% nvram_match("tinc_guest_enable", "1", "checked"); %>>是
												<input type="radio" value="0" name="tinc_enable" class="input" <% nvram_match("tinc_guest_enable", "0", "checked"); %>>否
-->
											</td>
										</tr>
<!--
										<tr id="tinc_url_tr">
											<th>远程配置地址</th>
											<td>
												<input type="text" maxlength="128" class="input_32_table" name="tinc_url" value="<% nvram_get("tinc_url"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
-->
										<tr id="tinc_id_tr">
											<th>设备ID</th>
											<td>
												<input type="text" class="devicepin" readonly="1" size=40 name="tinc_id" value="<% nvram_get("tinc_id"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
<!--
										<tr id="tinc_passwd_tr">
											<th>密码</th>
											<td>
												<input type="text" maxlength="16" class="input_15_table" name="tinc_passwd" value="<% nvram_get("tinc_passwd"); %>" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
-->
										</table>

									<div id="tinc_rulelist_Table_0" wanUnitID="0"></div>
									<div id="tinc_rulelist_Block_0" wanUnitID="0"></div>

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
