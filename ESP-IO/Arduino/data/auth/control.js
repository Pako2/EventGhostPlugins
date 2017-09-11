var websock;
var timezone;
var tr;
var doc;
var pinlist;
var head = ["Title","Direction","Value","Button"];
var values = ["LOW","HIGH"];
var coldirs = ["DodgerBlue","Red"];
var directions = ["In","Out"];
var password = "FAKE";
var source;
//var millisecondsToWait = 500;
var count=0;


//(function(){
//    var tId = setInterval(function(){if(document.readyState == "complete") onComplete()},50);
//    function onComplete(){
//        clearInterval(tId);    
//        console.log("loaded: ",new Date().getTime().toString());
//    };
//})()

$(window).load(function()
{
    console.log("Window loaded   ",new Date().getTime().toString());

        if (!!window.EventSource) {
           source = new EventSource('/events');
           source.addEventListener('open', function(e) {
              console.log("Events connected "+new Date().getTime().toString());
           }, false);
           source.addEventListener('error', function(e) {
              if (e.target.readyState != EventSource.OPEN) {
                 console.log("Events disconnected");
              }
            }, false);
            source.addEventListener('message', function(e)
            {
               password = e.data;
               //alert("message = "+password);
               console.log("Event message   ",new Date().getTime().toString());
               setWebsock();
               source.close();
            }, false);
        }     
    
});


function createTable()
{
    doc = document;
    var fragment = doc.createDocumentFragment();
    var thead = doc.createElement("thead");
    tr = doc.createElement("tr");
    tr.setAttribute("class", "warning");
    addRow(thead, "th", 0);
    fragment.appendChild(thead);

    var tbody = doc.createElement("tbody");
    for(i=0;i<pinlist.length;i++)
    {
      tr = doc.createElement("tr");
      addRow(tbody, "td", i);
    }
    fragment.appendChild(tbody);
    var table = doc.createElement("table");
    table.setAttribute("class", "table table-bordered table-condensed table-hover");
    table.appendChild(fragment);
    doc.getElementById("table_here").appendChild(table);
}

function addRow(dom, tag, ix)
    {
      var el;
      for(j=0;j<head.length;j++)
      {
        el = doc.createElement(tag);
        if (j > 0){el.setAttribute("class", "text-center");}
        if (tag == "th"){el.innerHTML = head[j];}
        else
        {
          if      (j===0) {el.innerHTML = pinlist[ix][0];}
          else if (j==1) 
          {
            var d = pinlist[ix][1] ? 1 : 0;
            el.innerHTML = directions[d];
          }
          else if (j==2) 
          {
            el.setAttribute("id", "value"+pinlist[ix][3]);
            if (pinlist[ix][3]=="A0")
            {
              el.innerHTML = pinlist[ix][2];
              el.style.backgroundColor ="PaleGreen";
            }
            else
            {
              el.innerHTML = values[pinlist[ix][2]];
              el.style.backgroundColor = coldirs[pinlist[ix][2]];
            }
          }
          else if (pinlist[ix][1]===true)
          {
            var b = doc.createElement("button");
            b.setAttribute("id", pinlist[ix][3]);
            b.setAttribute("onclick", "sendCommand(\"toggle\",{id:this.id})");
            b.setAttribute("class", "btn btn-primary btn-xs");
            b.innerHTML = "Toggle state";
            el.appendChild(b);
          }
        }
        tr.appendChild(el);
        dom.appendChild(tr);
      }
    }


function applyChange(id, value)
{
  var el = document.getElementById("value"+id);
  if (id=="A0")
  {
    el.innerHTML = value;
  }
  else
  {
    el.innerHTML = values[value];
    el.style.backgroundColor = coldirs[value];
  }  
}


function sendCommand(cmd, kwargs)
{
    var commandtosend = {};
    commandtosend.command = cmd;
        for (var k in kwargs) {
        if (kwargs.hasOwnProperty(k)) {
           commandtosend[k] = kwargs[k];
        }
    }
    websock.send(JSON.stringify(commandtosend));
}


function setWebsock()
{
  websock = new WebSocket("ws://" + window.location.hostname + "/ws");
  websock.onopen  = function(evt) {};
  websock.onclose = function(evt) {};
  websock.onerror = function(evt) {console.log(evt);};
  websock.onmessage = function(evt)
  {
    var obj = JSON.parse(evt.data);
    if (obj.command === "pinlist") 
  {
    pinlist = obj.pinstates;
    createTable();
    document.getElementById("loading-img").style.display = "none";
  }
  else if (obj.command === "change") 
  {
    applyChange(obj.id, obj.value);
  }
  else if (obj.command === "password")
  {
    websock.send("{\"command\":\"password\",\"password\":\""+password+"\"}");
  }
  else if (obj.command === "authorized")
  {
    var commandtosend = {};
    commandtosend.command = "pinlist";
    websock.send(JSON.stringify(commandtosend));
  }
  };
}


//function wait()
//{
//  console.log("Loop count: "+count.toString());
//  if (password == "FAKE")
//  {
//    count +=1;
//    if (count < 10) {setTimeout(function(){wait();}, millisecondsToWait);}
//    else {setWebsock();}
//  }
//  else
//  {
//    console.log("Password retrieved: "+new Date().getTime().toString());
//    source.close();
//    setWebsock();
//  }
//}
