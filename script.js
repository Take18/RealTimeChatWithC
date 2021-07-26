"use strict";

const detail_btn = document.getElementById("user_register_detail");
const name_submit = document.getElementById("name_submit");
const name_input = document.querySelector("input[name='name']");
const msg_list = document.getElementById("message_list");
const last_msg = document.querySelector("input[name='last_msg']");
const msg_input = document.querySelector("textarea[name='message']");
const user_id = document.querySelector("input[name='user_id']");
const msg_submit = document.getElementById("submit");
const xhr = new XMLHttpRequest();

detail_btn.addEventListener( 'click', function(e){
	alert("あなたの名前付きでメッセージを送信したい場合はユーザ登録を行ってください。\n※登録しない場合は匿名で送信されます。");
}, false );

name_submit.addEventListener( 'click', function(e){
	if ( name_input.value === "" ) {
		alert( "ユーザ名を入力してください。" );
		return;
	}
	ajaxPost("user", "name="+name_input.value, function(json){
		console.log(json);
		if ( json.result !== "success" ) {
			alert("ユーザ名が設定できませんでした。");
			return;
		}
		user_id.value = json.id;
		name_input.setAttribute("readonly", true);
		name_submit.style.display = "none";
	});
}, false );

msg_submit.addEventListener( 'click', function(e){
	if ( msg_input.value === "" ) {
		alert( "メッセージが入力されていません。" );
		return;
	}
	ajaxPost("send", "user="+user_id.value+"&content="+msg_input.value, function(json){
		console.log(json);
		if ( json.result !== "success" ) {
			alert("メッセージが送信できませんでした。");
			return;
		}
		msg_input.value = "";
	});
}, false );

setInterval(getRecentMessages, 1000);

function getRecentMessages() {
	const last_msg_id = last_msg.value;
	ajaxGet( "receive", "current="+last_msg_id, function(json){
		console.log(json);
		if ( json.result !== "success" ) {
			return false;
		}
		last_msg.value = json.last_msg;
		const msgs = json.messages;
		msgs.forEach(function(msg){
			let row = document.createElement("div");
			if ( user_id.value !== -1 && user_id === msg.user_id ) {
				row.classList.add("from_you");
			}
			row.innerHTML = "<span>"+msg.user_name+"</span> : <span>"+msg.content+"</span>";
			msg_list.append(row);
		});
	});
}

function ajaxPost( url, param, func ) {
	xhr.open( 'POST', url );
	xhr.setRequestHeader( 'content-type', 'application/x-www-form-urlencoded;charset=UTF-8' );
	xhr.send( param );
	xhr.onreadystatechange = function(){
		if ( xhr.readyState === 4 && xhr.status === 200 ) {
			console.log(xhr.responseText);
			func(JSON.parse(xhr.responseText));
		}
	};
}

function ajaxGet( url, param, func ) {
	xhr.open( 'GET', url+"?"+param );
	xhr.setRequestHeader( 'content-type', 'application/x-www-form-urlencoded;charset=UTF-8' );
	xhr.send( param );
	xhr.onreadystatechange = function(){
		if ( xhr.readyState === 4 && xhr.status === 200 ) {
			console.log(xhr.responseText);
			func(JSON.parse(xhr.responseText));
		}
	};
}
