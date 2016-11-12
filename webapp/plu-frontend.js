/*
 * Works with paclink-unix programs:
 *   wl2ktelnet, wl2kax25 or wl2kserial
 */
$(function () {
        "use strict";

        var WebIpSock="1339";
        /* Get server IP */
        var WebIpAddr = window.location.hostname;

        // for better performance - to avoid searching in DOM

        var input = $('#input');
        var wl2kargsin = $('#wl2kargsin');
        var status = $('#status');
        var wl2kWin = $('#wl2kwin');
        var debugWin = $('#debugwin');
        var outboxBtn = $('#outboxBtn');

        // maximum number of lines to keep before throwning them away
        var maxWindowItems = 200;
        // arguments for paclink-unix programs
        var wl2kargs = '';

        // if user is running mozilla then use it's built-in WebSocket
        window.WebSocket = window.WebSocket || window.MozWebSocket;

        // if browser doesn't support WebSocket, just show some notification and exit
        if (!window.WebSocket) {
                wl2kWin.html($('<p>', { text: 'Sorry, but your browser doesn\'t '
                + 'support WebSockets.'} ));
                input.hide();
                $('span').hide();
                return;
        }

        // open connection
        var connection = new WebSocket('ws://'+ WebIpAddr+':'+WebIpSock);

        connection.onopen = function () {
                input.removeAttr('disabled');
                status.text('web socket status:');
                input.val('Connection opened.');
        };

        connection.onerror = function (error) {
                // just in case there were problems with conenction...
                input.val('Problem with connection or the server is down.');
        };

        // most important part - incoming messages
        connection.onmessage = function (message) {
                // try to parse JSON message. Because we know that the server always returns
                // JSON this should work without any problem but we should make sure that
                // the massage is not chunked or otherwise damaged.
                try {
                        var json = JSON.parse(message.data);
                } catch (e) {
                        debugWin.append('This doesn\'t look like a valid JSON: type: ' + message.type + " data: " + message.data);
                        console.log('This doesn\'t look like valid JSON: ', message.data);
                        return;
                }

                if (json.type === 'msg') {
                        var lines = json.data.split(/\r\n|\r|\n/g);
//                        debugWin.append('<p>number of lines: ' + lines.length + '</p>');
                        for(var i=0; i < lines.length; i++) {
                                addWl2k(lines[i]);
                        }

//                        var json2 = json.data.replace(/\\n/g, '<BR>');

                } else  if (json.type === 'color') { // first response from the server with user's color

                        outboxBtn.css('background-color', json.data);
                } else {
                        debugWin.append('<p>Unhandled: ' + 'name: ' + json.type + '</p>' );
                        console.log('Hmm..., I\'ve never seen JSON like this: ', JSON.stringify(json));
                }
                wl2kargsin.removeAttr('disabled').focus();
                // User can now set another path

        };

        /**
	 * Get  args for paclink-unix programs:
	 */
        wl2kargsin.keydown(function(e) {
                /* Note this will not catch last character typed */
                wl2kargs = $(this).val();
                if (e.keyCode === 13) {

                        if (!wl2kargs) {
                                return;
                        }

                        connection.send(JSON.stringify( { type: 'wl2kargs', data: wl2kargs} ));
                }
        });

        document.getElementById("outboxBtn").onclick=function(){
                debugWin.append('<p>CheckOutbox</p>');
                /* Need to do this to get the last character typed */
                wl2kargsin.keydown();
                connection.send(JSON.stringify( { type: 'wl2kargs', data: wl2kargs} ));

                var msg = 'send_button_test';
                connection.send(JSON.stringify( { type: 'message', data: msg} ));
        };

        document.getElementById("sendTelnetBtn").onclick=function(){
                debugWin.append('<p>Send via Telnet</p>');
                wl2kargsin.keydown();
                connection.send(JSON.stringify( { type: 'wl2kargs', data: wl2kargs} ));

                var msg = 'send_button_telnet';
                connection.send(JSON.stringify( { type: 'message', data: msg} ));
        };

        document.getElementById("sendAX25Btn").onclick=function(){
                debugWin.append('<p>Send via AX.25</p>');
                wl2kargsin.keydown();
                connection.send(JSON.stringify( { type: 'wl2kargs', data: wl2kargs} ));

                var msg = 'send_button_ax25';
                connection.send(JSON.stringify( { type: 'message', data: msg} ));
        };

        document.getElementById("sendSerialBtn").onclick=function(){
                debugWin.append('<p>Send via Telnet</p>');
                wl2kargsin.keydown();
                connection.send(JSON.stringify( { type: 'wl2kargs', data: wl2kargs} ));

                var msg = 'send_button_serial';
                connection.send(JSON.stringify( { type: 'message', data: msg} ));
        };

        document.getElementById("sendCancelBtn").onclick=function(){
                debugWin.append('<p>Cancel</p>');
                var msg = 'send_button_kill_process';
                connection.send(JSON.stringify( { type: 'message', data: msg} ));
        };

        /**
         * This method is optional.
         * If the server wasn't able to respond in 3 seconds then display some error
         * message to notify the user.
         */
        setInterval(function() {
                if (connection.readyState !== 1) {
                        status.text('Error');
                        input.attr('disabled', 'disabled').val('Unable to communicate '
                                + 'with the WebSocket server.');
                }
        }, 3000);

        /**
	 * Add message to main window
	 */
        function addWl2k(message) {
                var pars = $("#wl2kwin div");
                var n = $("#wl2kwin").children().length;
//                debugWin.append('<p>' + 'count elements ' + pars.length + '  childs: ' + n + '</p>');

                wl2kWin.append('<div class="wl2k">'
                               + message + '</div>');
                wl2kWin.scrollTop(wl2kWin[0].scrollHeight);

                if (n > maxWindowItems) {
                        $("#wl2kwin").children("div:first").remove()
                }
        }

});
