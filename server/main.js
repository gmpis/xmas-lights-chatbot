'use strict';

const express = require("express");
const body_parser = require("body-parser");

const app = express().use(body_parser.json());
const port = (process.env.PORT || 5000);

// global vars for the get req
// WARNING currently status is stored only as a variable, if something happens eg server restarts, status will be reset
// you may want to store status in a db or on a file and restore it every time
var myStatus = "000"; //new status to send to arduino, init to 000 == all off
const myStatusStrLen = 3; //len of statusString == number of relay modules we want to control
var myDelay = (process.env.MY_DELAY || 10); //suggested delay before arduino sends next req

// print debug info if true
var myAllow = (process.env.ALLOW_PR || false);



// generates new_status string based on old status, users intent
// better in a separate module but remains here for readability
function genStatusStr(oldStatus,onlyToArray, mySymbol){

	let outArr = oldStatus.split(""); // create array from oldStatus string

	let inpArrLen = onlyToArray.length;


	if( inpArrLen === 0){
		// change the status of all the relay modules
		let counter;
		for(counter=0; counter < myStatusStrLen; counter++){
			//outArr.push(mySymbol);
			outArr[counter] = mySymbol;
		}

	}else{
		// edit only some modules, not all
		// simplistic approach for only 3 modules

		if(onlyToArray.includes("first")){
			//change the status of the first module to mySymbol
			outArr[0] = mySymbol;
		}

		if(onlyToArray.includes("second")){
			outArr[1] =mySymbol;
		}

		if(onlyToArray.includes("third")){
			outArr[2] = mySymbol;
		}

	}


	let outStr = "";

	// convert outArr to string and return
	let mCounter;
	for(mCounter=0; mCounter < myStatusStrLen; mCounter++){
		outStr =  outStr + outArr[mCounter];
	}

	return outStr;
}


app.get("/", function(req,res){
	res.send("Hello world");
});

app.post("/dialogflow", function(req,res){
	let body = req.body;

	if(myAllow){ //ONLY FOR DEBUGGING REMOVE PLEASE
		console.log("headers: "+ JSON.stringify(req.headers));
		console.log("body: "+ JSON.stringify(body));	
	}


	// could do naive authentication check here, using additional headers
	// req.get("Dialog-Secret") or req.header(name)


	let my_resp = "?"; // send this if request isnt from dialogflow

	if(body.result && body.result.action){
		// here if the req is from dialogflow and about the on/off intents

		let action = body.result.action;
		console.log('action: '+action);
		let onlyOnRelModules =[];
		let old_stat = myStatus;

	
		if (body.result.parameters['set_index']) {
			onlyOnRelModules = body.result.parameters['set_index'];
			//console.log('Len: '+onlyOnRelModules.length);
		}

		if(action === "status.off"){
			myStatus = genStatusStr(old_stat, onlyOnRelModules,"0");
			my_resp = " Deactivating ...";

		}else if(action === "status.on"){
			myStatus = genStatusStr(old_stat, onlyOnRelModules,"1");
			my_resp = "Activating ...";
		}else{
			my_resp = "L: I didn't get that. Can you say it again?";
		}


		//ONLY FOR DEBUGGING REMOVE PLEASE
		if(myAllow){
			let debPr = "\n OLD STATUS: "+old_stat+", with action: "+action+", index: "+JSON.stringify(onlyOnRelModules)+", NEW STATUS: "+myStatus+"\n";
			console.log(debPr);
		}
	}

	res.status(200).json({"speech":my_resp,"displayText":my_resp});
});

app.get("/arduino", function(req,res){
	// response is in csv format

	//1. current time in milliseconds
	//2. length of new_status string
	//3. new status for relay modules , len must be equal to previous field (2)
	//4. suggested  delay before arduino requests next update, so we can adjust
	//   the frequency over the air, without the need to program the board again

	let out_str = "";
	let myMili = Date.now();


	out_str = myMili+","+myStatusStrLen+","+myStatus+","+myDelay;// last 3 are global

	// set Content-Type header
	res.set("Content-Type","text/plain");
	res.status(200).send(out_str);

});


app.listen(port, function(){
	console.log("Listening on "+port+"...");

});

