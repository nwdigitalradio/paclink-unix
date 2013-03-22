// http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
"use strict";

// Optional. You will see this name in eg. 'ps' or 'top' command
process.title = 'plu-server';

var fs = require('fs');
var exec = require('child_process').exec;

//var plu_command = "/home/gunn/bin/coretemp.sh";
var plu_command = "/usr/local/bin/wl2ktelnet";
var plu_args = '';

//var plu_command = "ls";
//var plu_args = '-salt';

// *** Network socket server
var net = require('net');
var NETHOST = '127.0.0.1';
//var NETPORT = 9124;
var UNIXPORT = '/tmp/PACLINK_UI';
var NETPORT = UNIXPORT;
var ETH_IFACE = 'br0';

// *** Web socket server
var webSocketsServerPort = 1339;

// *** HTML server
var HTMLPORT = 8082;

var TCP_DELIMITER = '\n';
var TCP_BUFFER_SIZE = Math.pow(2,16);


/**
 * Global variables
 */
// list of currently connected clients (users)
var pluClients = [];

var netClients = [ ];
var wsClients = [ ];
var myIPaddr=0;


// websocket and http servers
var webSocketServer = require('websocket').server;
var http = require('http');
var events = require("events");

var aprs_emitter = new events.EventEmitter();
//var msg_emitter = new events.EventEmitter();

/**
 * Helper function for escaping html special characters
 */
function htmlEntities(str) {
        return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;')
                        .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
};

function exec_command (chillens, connection) {
        var sys = require('sys');
        var cached;
        var command = 'ls -lt /usr/local/var/wl2k/outbox';
        //        var command = 'tail -f /var/log/syslog';

        console.log('Yeah, button depressed\n');

        var new_child = exec(command, function (error, stdout, stderr) {
                sys.print('stdout: ' + stdout);
                var dataStr = stdout.toString('utf8');

                var jmsg = {
                        type: 'msg',
                              data: dataStr
                };
                var jmsgstr = JSON.stringify( jmsg );

                console.log('json msg: ' + jmsgstr + '\n');

                connection.sendUTF(jmsgstr);

                if(stderr !== null) {
                        sys.print('stderr: ' + stderr);
                }
                if (error !== null) {
                        console.log('exec error: ' + error);
                }
        });

        chillens.push(new_child);

        new_child.on('exit', function() {
                var index = chillens.indexOf(new_child);
                chillens.splice(index, 1);
                console.log('process 1 exit, remaining processes: ' + chillens.length + '\n');
        });
};

function spawn_command (chillens, connection, progname) {
        var child_process = require('child_process');
        var cached;
        var plu_uid = 1002;
        var plu_gid = 1002;

        console.log(' button depressed for: ' + progname + ', strlen args: ' + plu_args.length + '\n');

        if(plu_args.length == 0) {
                console.log('child_process: ' + progname + ' with no args\n');
                var new_child = child_process.spawn(progname, [], {
                        cwd: '.',
                             uid: plu_uid,
                             gid: plu_gid
                });
        } else {
                console.log('child_process: ' + progname + ' with args: ' + plu_args + '\n');
                var new_child = child_process.spawn(progname, [plu_args], {
                        cwd: '.',
                             uid: plu_uid,
                             gid: plu_gid
                });
        }
        chillens.push(new_child);


        new_child.on('exit', function() {
                var index = chillens.indexOf(new_child);
                chillens.splice(index, 1);
                console.log('process exit 2, remaining processes: ' + chillens.length + '\n');
        });

        new_child.stdout.on('data', function(data) {
                process.stdout.write('' + data);
                var dataStr = htmlEntities(data.toString('utf8'));

                var jmsg = {
                        type: 'msg',
                              data: dataStr
                };
                var jmsgstr = JSON.stringify( jmsg );

                console.log('json msg stdout: ' + jmsgstr + '\n');

                connection.sendUTF(jmsgstr);

        });

        new_child.stderr.on('data', function(data) {
                process.stderr.write('' + data);
                var dataStr = htmlEntities(data.toString('utf8'));

                var jmsg = {
                        type: 'msg',
                              data: dataStr
                };
                var jmsgstr = JSON.stringify( jmsg );

                console.log('json msg stderr: ' + jmsgstr + '\n');

                connection.sendUTF(jmsgstr);


        });
};

var getNetworkIP = (function () {
        var ignoreRE = /^(127\.0\.0\.1|::1|fe80(:1)?::1(%.*)?)$/i;

        var cached;
        var command;
        var filterRE;

        switch (process.platform) {

        case 'darwin':
                command = 'ifconfig ' + ETH_IFACE;
                filterRE = /\binet\s+([^\s]+)/g;
                // filterRE = /\binet6\s+([^\s]+)/g; // IPv6
                              break;
        default:
                command = 'ifconfig ' + ETH_IFACE;
                filterRE = /\binet\b[^:]+:\s*([^\s]+)/g;
                // filterRE = /\binet6[^:]+:\s*([^\s]+)/g; // IPv6
                              break;
        }

        return function (callback, bypassCache) {
                // get cached value
                if (cached && !bypassCache) {
                        callback(null, cached);
                        return;
                }
                // system call
                exec(command, function (error, stdout, sterr) {
                        var ips = [];
                        // extract IPs
                                        var matches = stdout.match(filterRE);
                        // JS has no lookbehind REs, so we need a trick
                        for (var i = 0; i < matches.length; i++) {
                                ips.push(matches[i].replace(filterRE, '$1'));
                        }

                        // filter BS
                        for (var i = 0, l = ips.length; i < l; i++) {
                                if (!ignoreRE.test(ips[i])) {
                                        //if (!error) {
                                                cached = ips[i];
                                                //}
                                        callback(error, ips[i]);
                                        return;
                                }
                        }
                        // nothing found
                        callback(error, null);
                });
        };
})();

/*
   * Broadcast server
   */
getNetworkIP(function (error, ip) {
        console.log(ip);
        if (error) {
                console.log('error:', error);
        } else {
                myIPaddr = ip;
        }
        console.log('Saving local ip address: ' + myIPaddr);
        return ip;
}, false);




// Array with some colors, don't use the background color, currently orange
   var colors = [ 'red', 'green', 'blue', 'magenta', 'purple', 'plum', 'GreenYellow', 'DarkKhaki', 'Brown', 'SaddleBrown', 'SteelBlue' ];
// ... in random order
   colors.sort(function(a,b) { return Math.random() > 0.5; } );

fs.unlink(UNIXPORT, function(err) {
        if (err) {
                console.log('Will create socket:' + UNIXPORT );
        } else {
                console.log('successfully deleted:' + UNIXPORT );
        };
});



/**
   * HTTP server
   */
var server = http.createServer(function(request, response) {
        // Not important for us. We're writing WebSocket server, not HTTP server
        });

server.listen(webSocketsServerPort, function() {
        console.log((new Date()) + " Server is listening on port " + webSocketsServerPort);
});

aprs_emitter.on("spy_display", function(message) {

        //        console.log("emitter.on called.");

        if(pluClients[0] !== undefined && pluClients.length !== undefined) {

                console.log('Sending to '+  pluClients.length + ' clients');
                var json1 = JSON.parse(message);
                var json = JSON.stringify({ type:'aprs', data: message });

                for (var i=0; i < pluClients.length; i++) {
                        //                        console.log('aprs_emitter: sending to ' +  i
                        //                        + ' total: '+ pluClients.length);
                        pluClients[i].sendUTF(message);

                }
        }
});

/**
 * ===================== WebSocket server ========================
 */
var wsServer = new webSocketServer({
        // WebSocket server is tied to a HTTP server. WebSocket request is just
        // an enhanced HTTP request. For more info http://tools.ietf.org/html/rfc6455#page-6
        httpServer: server
});

wsClients.push(wsServer);

// This callback function is called every time someone
// tries to connect to the WebSocket server
wsServer.on('request', function(request) {
        var chillens = [];

        console.log((new Date()) + ' Connection from origin ' + request.origin + '.');

        // accept connection - you should check 'request.origin' to make sure that
        // client is connecting from your website
        // (http://en.wikipedia.org/wiki/Same_origin_policy)
        var connection = request.accept(null, request.origin);
        // we need to know client index to remove them on 'close' event
        var index = pluClients.push(connection) - 1;
        var curIndex =0;
        var userName = false;
        var destName = false;
        var userColor = false;

        console.log((new Date()) + ' Connection accepted for index: ' + index);

        //Test if anyone has logged in yet
        if(pluClients[0] !== undefined && pluClients[0].length !== undefined) {
                console.log("ws: Client length: " +  pluClients[0].length );
        } else {
                console.log("ws: NO Client");
        }

        console.log('Test front end');

        // user sent some message
        connection.on('message', function(message) {

                console.log("connection: received message");

                if (message.type === 'utf8') { // accept only text

                        /* get data sent from users web page */
                        console.log('Parse message: ' + message.utf8Data);

                        var frontendmsg = JSON.parse(message.utf8Data);
                        console.log('Parse message 2: ' + frontendmsg.type);

                        if(frontendmsg.type === "config") {
                                var json = JSON.stringify({ type:'config', data: frontendmsg.data});
                                console.log('Sending config to radio: length =' + json.length + ' data: ' + frontendmsg.data);

                                msg_emitter.emit("aprs_cfg", json);
                        }  else if(frontendmsg.type === "message") {

                                // log and broadcast the message
                                console.log((new Date()) + ' Received Message from '
                                            + userName + ': ' + frontendmsg.data);

                                if(frontendmsg.data === "send_button_test") {

                                        exec_command(chillens, connection);

                                } else if(frontendmsg.data === "send_button_telnet") {

                                        spawn_command(chillens, connection, "wl2ktelnet");

                                } else if(frontendmsg.data === "send_button_ax25") {

                                        spawn_command(chillens, connection, "wl2kax25");

                                } else if(frontendmsg.data === "send_button_serial") {

                                        spawn_command(chillens, connection, "wl2kserial");

                                } else if (frontendmsg.data === "send_button_kill_process") {

                                        if(chillens.length != 0) {
                                                killWorkers();
                                        } else {
                                                console.log("kill button pushed but no child processes running\n");
                                        }
                                }

                        } else if (frontendmsg.type === "wl2kargs") {

                                plu_args = frontendmsg.data;
                                console.log("connection: received wl2kargs");

                                /* get data sent from users web page */
                                console.log('Parse wl2kargs: ' + plu_args);

                        } else {
                                console.log('Unhandled message type from client: ' + frontendmsg.type);
                        }
                }
        });

        var killWorkers = function() {
                chillens.forEach(function(child) {
                        if (typeof child !== 'undefined') {
                                process.kill(child.pid, 'SIGINT');
                                var index = chillens.indexOf(child);
                                chillens.splice(index, 1);

                                console.log('killed process: ' + child.pid + ', remaining processes: ' + chillens.length + '\n');
                        }
                });
        };

        // user disconnected
        connection.on('close', function(connection) {
                if (userName !== false && userColor !== false) {
                        console.log((new Date()) + " Peer "
                                    + connection.remoteAddress + " disconnected.");
                        // remove user from the list of connected clients
                        pluClients.splice(index, 1);
                        // push back user's color to be reused by another user
                        colors.push(userColor);
                }
        });

        fs.watch('/usr/local/var/wl2k/outbox/', function(event, filename) {
                var fileNames = fs.readdirSync('/usr/local/var/wl2k/outbox/');

                console.log('fs watch: event: ' + event + ' filename: ' + filename + ' Number of files: ' + fileNames.length);

                /* Change button color if there are any files in the
                 * outbox.*/
                var btnColor = (fileNames.length > 0) ? 'green' : '';
                connection.sendUTF(JSON.stringify({ type:'color', data: btnColor }));
        });
});

/**
 * ===================== network Socket server ========================
 *
 * Create a server instance, and chain the listen function to it
 * The function passed to net.createServer() becomes the event handler for the 'connection' event
 * The sock object the callback function receives UNIQUE for each connection
 */
net.createServer(function(sock) {


        // We have a connection - a socket object is assigned to the connection automatically
        console.log('CONNECTED: ' + sock.remoteAddress +':'+ sock.remotePort);

        // To buffer tcp data see:
        // http://stackoverflow.com/questions/7034537/nodejs-what-is-the-proper-way-to-handling-tcp-socket-streams-which-delimiter
        var buf = new Buffer(TCP_BUFFER_SIZE);  //new buffer with size 2^16

        //	sock.emit(news, {hello: world});
        netClients.push(sock);

        if(pluClients[0] !== undefined && pluClients[0].length !== undefined) {
                console.log("net0: Chat Client length: " +  pluClients[0].length );
        } else {
                console.log("net0: NO Chat Client");
        }
        var icount = 0;

        // Add a 'data' event handler to this instance of socket
        sock.on('data', function(data) {
                var dataStr = data.toString('utf8');
                var aprs_raw = "";

                // Stringify the aprs_disp object
                var aprs_disp = {
                        "type": "spy",
                        "value": dataStr
                }

                var aprsdisp_json = JSON.stringify(aprs_disp);
                //			console.log("aprs_disp: " + aprsdisp_json );
                icount++;
                aprs_emitter.emit("spy_display", aprsdisp_json);
                console.log('DATA [' + icount + "]: len: " + data.length + '  ' + data.toString('utf8'));
        });

        // Add a 'close' event handler to this instance of socket
        sock.on('close', function(data) {
                console.log('Unix socket connection closed at: ' + (new Date()) + 'socket address: ' + sock.remoteAddress +' '+ sock.remotePort);
        });

        //        msg_emitter.on("aprs_msg", function(message) {
        //                console.log('MSG: ' + message);
        //                sock.write(message);
        //        });

        //        msg_emitter.on("aprs_cfg", function(message) {
        //                console.log('CFG: ' + message);
        //                sock.write(message);
        //        });

}).listen(NETPORT, NETHOST);

console.log('Server listening on ' + NETHOST +':'+ NETPORT);

/**
 * ===================== HTML server ========================
 */

var connect = require('connect');
connect.createServer(
                     connect.static(__dirname),
                     function(req, res){
        res.setHeader('Content-Type', 'text/html');
        res.end('You need a path, try /plu.html\n');
}
).listen(HTMLPORT);
