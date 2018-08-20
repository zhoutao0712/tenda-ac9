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
var add_wanIp_array = new Array();
var add_lanIp_array = new Array();

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
	gen_tinc_ruleTable_Block("tinc_rulelist_0");
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_0"], "tinc_rulelist_0");

	tinc_rulelist_array["tinc_rulelist_1"] = parseNvramToArray('<% nvram_char_to_ascii("","tinc_wan_ip"); %>');
	gen_tinc_ruleTable_Block("tinc_rulelist_1");
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_1"], "tinc_rulelist_1");

	tinc_rulelist_array["tinc_rulelist_2"] = parseNvramToArray('<% nvram_char_to_ascii("","tinc_lan_ip"); %>');
	gen_tinc_ruleTable_Block("tinc_rulelist_2");
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_2"], "tinc_rulelist_2");
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

function validRuleForm(_tableID){
	if(_tableID == "tinc_rulelist_0") {
		add_ruleList_array = [];
		add_ruleList_array.push(document.getElementById("tinc_action_x_0").value);
		add_ruleList_array.push(document.getElementById("tinc_host_x_0").value);
	}
	if(_tableID == "tinc_rulelist_1") {
		add_wanIp_array = [];
		add_wanIp_array.push(document.getElementById("tinc_action_x_1" ).value);
		add_wanIp_array.push(document.getElementById("tinc_host_x_1").value);
	}

	if(_tableID == "tinc_rulelist_2") {
		add_lanIp_array = [];
		add_lanIp_array.push(document.getElementById("tinc_action_x_2" ).value);
		add_lanIp_array.push(document.getElementById("tinc_host_x_2").value);
	}

	return true;
}

function addRow_host(upper){
	if(validRuleForm("tinc_rulelist_0")){
		var rule_num = tinc_rulelist_array["tinc_rulelist_0"].length;
		if(rule_num >= upper){
			alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
			return false;
		}

		var tinc_rulelist_array_temp = tinc_rulelist_array["tinc_rulelist_0"].slice();
		var add_ruleList_array_temp = add_ruleList_array.slice();
		if(tinc_rulelist_array_temp.length > 0) {
			tinc_rulelist_array["tinc_rulelist_0"].push(add_ruleList_array);
		}
		else {
			tinc_rulelist_array["tinc_rulelist_0"].push(add_ruleList_array);
		}

		document.getElementById("tinc_action_x_0").value = "+";
		document.getElementById("tinc_host_x_0").value = "";
		showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_0"], "tinc_rulelist_0");
		return true;
	}
}

function addRow_wanip(upper){
		if(validRuleForm("tinc_rulelist_1")){
			var rule_num = tinc_rulelist_array["tinc_rulelist_1"].length;
			if(rule_num >= upper){
				alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
				return false;
			}

			var tinc_rulelist_array_temp = tinc_rulelist_array["tinc_rulelist_1"].slice();
			var add_ruleList_array_temp = add_wanIp_array.slice();
			if(tinc_rulelist_array_temp.length > 0) {
				tinc_rulelist_array["tinc_rulelist_1"].push(add_wanIp_array);
			}
			else {
				tinc_rulelist_array["tinc_rulelist_1"].push(add_wanIp_array);
			}

			document.getElementById("tinc_action_x_1").value = "+";
			document.getElementById("tinc_host_x_1").value = "";
			showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_1"], "tinc_rulelist_1");
			return true;
		}
}

function addRow_lanip(upper){
		if(validRuleForm("tinc_rulelist_2")){
			var rule_num = tinc_rulelist_array["tinc_rulelist_2"].length;
			if(rule_num >= upper){
				alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
				return false;
			}

			var tinc_rulelist_array_temp = tinc_rulelist_array["tinc_rulelist_2"].slice();
			var add_ruleList_array_temp = add_lanIp_array.slice();
			if(tinc_rulelist_array_temp.length > 0) {
				tinc_rulelist_array["tinc_rulelist_2"].push(add_lanIp_array);
			}
			else {
				tinc_rulelist_array["tinc_rulelist_2"].push(add_lanIp_array);
			}

			document.getElementById("tinc_action_x_2").value = "1";
			document.getElementById("tinc_host_x_2").value = "";
			showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_2"], "tinc_rulelist_2");
			return true;
		}
}

function del_Row_host(row_idx){
	tinc_rulelist_array["tinc_rulelist_0"].splice(row_idx, 1);
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_0"], "tinc_rulelist_0");
}

function del_Row_wanip(row_idx){
	tinc_rulelist_array["tinc_rulelist_1"].splice(row_idx, 1);
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_1"], "tinc_rulelist_1");
}

function del_Row_lanip(row_idx){
	tinc_rulelist_array["tinc_rulelist_2"].splice(row_idx, 1);
	showtinc_rulelist(tinc_rulelist_array["tinc_rulelist_2"], "tinc_rulelist_2");
}

function showtinc_rulelist(_arrayData, _tableID) {
	if(_tableID == "tinc_rulelist_0") {
		var width_array = [14, 75, 11];
		var handle_long_text = function(_len, _text, _width) {
			var html = "";
			if(_text.length > _len) {
				var display_text = "";
				display_text = _text.substring(0, (_len - 2)) + "...";
				html +='<td width="' +_width + '%" title="' + _text + '">' + display_text + '</td>';
			}
			else {
				html +='<td width="' + _width + '%">' + _text + '</td>';
			}

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
								code += handle_long_text(32, eachValue[j], width_array[j]);
								break;
							case 2 :
								code += handle_long_text(14, eachValue[j], width_array[j]);
								break;
							default :
								code +='<td width="' + width_array[j] + '%">' + eachValue[j] + '</td>';
								break;
						}
					}

					code +='<td width="14%"><input class="remove_btn" onclick="del_Row_host(' + i + ');" value=""/></td></tr>';
					code +='</tr>';
				}
			}
		}

		code +='</table>';
		document.getElementById("tinc_rulelist_Block_0").innerHTML = code;
	}

	if(_tableID == "tinc_rulelist_1") {
		var width_array = [14, 75, 11];
		var handle_long_text = function(_len, _text, _width) {
			var html = "";
			if(_text == "+") _text = "启用加速";
			if(_text == "-") _text = "禁用加速";
			if(_text.length > _len) {
				var display_text = "";
				display_text = _text.substring(0, (_len - 2)) + "...";
				html +='<td width="' +_width + '%" title="' + _text + '">' + display_text + '</td>';
			}
			else {
				html +='<td width="' + _width + '%">' + _text + '</td>';
			}

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

					code +='<td width="14%"><input class="remove_btn" onclick="del_Row_wanip(' + i + ');" value=""/></td></tr>';
					code +='</tr>';
				}
			}
		}

		code +='</table>';
		document.getElementById("tinc_rulelist_Block_1").innerHTML = code;
	}

	if(_tableID == "tinc_rulelist_2") {
		var width_array = [14, 75, 11];
		var handle_long_text = function(_len, _text, _width) {
			var html = "";
			if(_text == "1") _text = "启用全局";
			if(_text == "2") _text = "禁用加速";
			if(_text.length > _len) {
				var display_text = "";
				display_text = _text.substring(0, (_len - 2)) + "...";
				html +='<td width="' +_width + '%" title="' + _text + '">' + display_text + '</td>';
			}
			else {
				html +='<td width="' + _width + '%">' + _text + '</td>';
			}

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

					code +='<td width="14%"><input class="remove_btn" onclick="del_Row_lanip(' + i + ');" value=""/></td></tr>';
					code +='</tr>';
				}
			}
		}

		code +='</table>';
		document.getElementById("tinc_rulelist_Block_2").innerHTML = code;
	}
}

function gen_tinc_ruleTable_Block(_tableID) {
	if(_tableID == "tinc_rulelist_0") {
		var html = "";
		html += '<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable_table">';
		html += '<thead>';
		html += '<tr>';

		html += '<td colspan="7">' + '自定义域名规则&nbsp;(<#List_limit#>&nbsp;128)</td>';
		html += '</tr>';
		html += '</thead>';

		html += '<tr>';
		html += '<th width="14%">动作</th>';

		html += '<th width="75%"><a class="hintstyle" href="javascript:void(0);" onClick="">域名</a></th>';
		html += '<th width="11%"><#list_add_delete#></th>';
		html += '</tr>';

		html += '<tr>';

		html += '<td width="14%">';
		html += '<select name="tinc_action_x_0" id="tinc_action_x_0" class="input_option">';
		html += '<option value="+">增加到列表</option>';
		html += '<option value="-">从列表删除</option>';
		html += '</select>';
		html += '</td>';

		html += '<td width="75%">';
		html += '<input type="text" maxlength="128" class="input_32_table" name="tinc_host_x_0" id="tinc_host_x_0" onKeyPress="return validator.isString(this, event)" autocorrect="off" autocapitalize="off"/>';
		html += '</td>';

		html += '<td width="11%">';
		html += '<input type="button" class="add_btn" onClick="addRow_host(128);" name="tinc_rulelist_0" id="tinc_rulelist_0" value="">';
		html += '</td>';

		html += '</tr>';
		html += '</table>';
		document.getElementById("tinc_rulelist_Table_0").innerHTML = html;
	}
	if(_tableID == "tinc_rulelist_1") {
		var html = "";
		html += '<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable_table">';
		html += '<thead>';
		html += '<tr>';

		html += '<td colspan="7">' + '自定义广域网IP规则&nbsp;(<#List_limit#>&nbsp;128)</td>';
		html += '</tr>';
		html += '</thead>';

		html += '<tr>';
		html += '<th width="14%">动作</th>';

		html += '<th width="75%"><a class="hintstyle" href="javascript:void(0);" onClick="">广域网IP</a></th>';
		html += '<th width="11%"><#list_add_delete#></th>';
		html += '</tr>';

		html += '<tr>';

		html += '<td width="14%">';
		html += '<select name="tinc_action_x_1" id="tinc_action_x_1" class="input_option">';
		html += '<option value="+">启用加速</option>';
		html += '<option value="-">禁用加速</option>';
		html += '</select>';
		html += '</td>';

		html += '<td width="75%">';
		html += '<input type="text" maxlength="128" class="input_32_table" name="tinc_host_x_1" id="tinc_host_x_1" onKeyPress="return validator.isString(this, event)" autocorrect="off" autocapitalize="off"/>';
		html += '</td>';

		html += '<td width="11%">';
		html += '<input type="button" class="add_btn" onClick="addRow_wanip(128);" name="tinc_rulelist_1" id="tinc_rulelist_1" value="">';
		html += '</td>';

		html += '</tr>';
		html += '</table>';
		document.getElementById("tinc_rulelist_Table_1").innerHTML = html;
	}

	if(_tableID == "tinc_rulelist_2") {
		var html = "";
		html += '<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable_table">';
		html += '<thead>';
		html += '<tr>';

		html += '<td colspan="7">' + '自定义局域网IP规则&nbsp;(<#List_limit#>&nbsp;128)</td>';
		html += '</tr>';
		html += '</thead>';

		html += '<tr>';
		html += '<th width="14%">动作</th>';

		html += '<th width="75%"><a class="hintstyle" href="javascript:void(0);" onClick="">局域网IP</a></th>';
		html += '<th width="11%"><#list_add_delete#></th>';
		html += '</tr>';

		html += '<tr>';

		html += '<td width="15%">';
		html += '<select name="tinc_action_x_2" id="tinc_action_x_2" class="input_option">';
		html += '<option value="1">启用全局模式</option>';
		html += '<option value="2">禁用加速模式</option>';
		html += '</select>';
		html += '</td>';

		html += '<td width="75%">';
		html += '<input type="text" maxlength="128" class="input_32_table" name="tinc_host_x_2" id="tinc_host_x_2" onKeyPress="return validator.isString(this, event)" autocorrect="off" autocapitalize="off"/>';
		html += '</td>';

		html += '<td width="11%">';
		html += '<input type="button" class="add_btn" onClick="addRow_lanip(128);" name="tinc_rulelist_2" id="tinc_rulelist_2" value="">';
		html += '</td>';

		html += '</tr>';
		html += '</table>';
		document.getElementById("tinc_rulelist_Table_2").innerHTML = html;
	}
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
		document.form.tinc_wan_ip.value = parseArrayToNvram(tinc_rulelist_array["tinc_rulelist_1"]);
		document.form.tinc_lan_ip.value = parseArrayToNvram(tinc_rulelist_array["tinc_rulelist_2"]);

		showLoading();
		document.form.submit();
	}
}

function validHost(enDomain){
	var checkOK = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-0123456789. "
	var ch;
	var i, j;
	if(enDomain=="" || enDomain==" " || enDomain.length < 4) {
//		alert("请输入合法的域名");
		return false;
	}

	for (i = 0; i < enDomain.length;  i++) {
		ch = enDomain.charAt(i);
		for (j = 0; j < checkOK.length; j++) {
			if (ch == checkOK.charAt(j)) break;
		}
		if (j == checkOK.length) {
//			alert("请输入合法的域名");
			return false;
		}
	}
	return true;
}

function validForm(){
	var i;
	var host_dataArray = tinc_rulelist_array["tinc_rulelist_0"];
	for(i = 0; i < host_dataArray.length; i += 1) {
		if(!validHost(host_dataArray[i][1])) {
			alert("请输入合法的域名")
			return false;
		}
	}

	var wanip_dataArray = tinc_rulelist_array["tinc_rulelist_1"];
	for(i = 0; i < wanip_dataArray.length; i += 1) {
		if(ipFilterZero(wanip_dataArray[i][1]) == -2) {
			alert("请输入合法的IP地址")
			return false;
		}
	}

	var lanip_dataArray = tinc_rulelist_array["tinc_rulelist_2"];
	for(i = 0; i < lanip_dataArray.length; i += 1) {
		if(ipFilterZero(lanip_dataArray[i][1]) == -2) {
			alert("请输入合法的IP地址")
			return false;
		}
	}

	return true;
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
<input type="hidden" name="tinc_wan_ip" value=''>
<input type="hidden" name="tinc_lan_ip" value=''>

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
										<tr>
											<th>主动重连时间间隔（秒，默认3600）</th>
											<td>
												<input type="text" maxlength="6" name="tinc_recon_seconds" class="input_15_table" value="<% nvram_get("tinc_recon_seconds"); %>" onKeyPress="return validator.isNumber(this,event)" autocorrect="off" autocapitalize="off">
											</td>
										</tr>

									</table>

									<div id="tinc_rulelist_Table_0"></div>
									<div id="tinc_rulelist_Block_0"></div>
									<div id="tinc_rulelist_Table_1"></div>
									<div id="tinc_rulelist_Block_1"></div>
									<div id="tinc_rulelist_Table_2"></div>
									<div id="tinc_rulelist_Block_2"></div>

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
