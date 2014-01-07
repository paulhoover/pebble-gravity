function appMessageAck(e) {
    console.log('options sent to Pebble successfully');
}

function appMessageNack(e) {
    console.log('options not sent to Pebble: ' + e.error.message);
}

Pebble.addEventListener('showConfiguration', function() {
    console.log('showing configuration');
    Pebble.openURL('http://hardy.dropbear.id.au/pebble/Gravity/config/2-0.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
    console.log('configuration closed');
    if (e.respone != '') {
	var params = JSON.parse(decodeURIComponent(e.response));
	console.log(JSON.stringify(params));
	window.localStorage.setItem('facestyle', params['facestyle']);
	Pebble.sendAppMessage(params, appMessageAck, appMessageNack);
    } else {
	console.log('no options received');
    }
});
