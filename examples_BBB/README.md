rf24_bbb is a program for BBB Node.JS developer.

### Usage:

	Usage: rf24_bbb [-w target] [[-c channel] -r read_addr] ...
		-D	show module detail
		-h	show this help
		-w	target	set target address
		-r	read addr	set read address
		-c	channel	set read channel
		-R[timeout]	read string
		-S	string	send string

### Node.JS Example

	var exec = require('child_process').exec;
	// rf24_bbb binary file
	var _nrflib = __dirname + "/rf24_bbb";
	// set RF24 writing pipe address to 2NODE and reading pipe 1NODE.
	var nrf = exec(_nrflib + " " + "-w 2Node -r 1Node -D");

	// Send string "Hello" per second
	setInterval(function(){
	    var x = exec(_nrflib + " " + "-S Hello");
	    x.stdout.on('data', function (data)
	    {
	        console.log(data);
	    });
	}, 1000);

	nrf.stdout.on('data', function (data)
	{
	    console.log(data);
	});

	nrf.stderr.on('data', function (data)
	{
	    console.log(data);
	});

See http://tmrh20.github.io/RF24 for more information