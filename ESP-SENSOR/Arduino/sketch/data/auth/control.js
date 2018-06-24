var websock;
//var doc;
var srcs = ["/images/OFF256-72.png", "/images/ON256-72.png"];
var password = "FAKE";
var source;
var count=0;
var txts = ["Turn ON", "Turn OFF"];
var classes = ["btn-success", "btn-danger"];
var lastTimeStamp = 0;




function test() {
    //var ts = Date.now();
    //console.log("Time stamp: ", Date.now().toString());
    var reload = document.getElementById("reload");
    if (Date.now()-lastTimeStamp>40000)
      {
        console.log("Nop missing !");
        reload.style.display = 'block';
      }
    else
      {
        reload.style.display = 'none';
      }
    }



function start()
{
      console.log("Window loaded   ",new Date().getTime().toString());


        Modal = function(el, options) {
        var self = this;
        
        this.el = document.querySelector(el);
        this.options = options;
        
        try {
            var list = document.querySelectorAll('#'+this.el.id+' [data-dismiss="modal"]');
            for (var x = 0; x < list.length; x++){
                list[x].addEventListener('click', function(e){ 
                    if(e) e.preventDefault();
                    self.hide();
                });
            }
        }
        catch(e){
            console.log(e);
        }
    };  
    
     /**
     * Methods
     */
     Modal.prototype.show = function() {
        var self = this;
        // adding backdrop (transparent background) behind the modal
        if (self.options.backdrop) {
            var backdrop = document.getElementById('bs.backdrop');
            if (backdrop === null) {
                backdrop = document.createElement('div');
                backdrop.id = "bs.backdrop";
                backdrop.className = "modal-backdrop fade in";
                document.body.appendChild(backdrop);
            }
        }
        // show modal
        this.el.classList.add('in');
        this.el.style.display = 'block';
        document.body.style.paddingRight = '13px';
        document.body.classList.add('modal-open');
    };
    
        Modal.prototype.hide = function() {
        var self = this;
        // hide modal
        this.el.classList.remove('in');
        this.el.style.display = 'none';
        document.body.style = '';
        document.body.classList.remove('modal-open');

        // removing backdrop
        if (self.options.backdrop) {
            var backdrop = document.getElementById('bs.backdrop');
            if (backdrop !== null) document.body.removeChild(backdrop);
        }
    };
    var m = new Modal('#myModal', {backdrop: true});
    document.getElementById('btn-open').addEventListener('click', function(e) {
      if(e) e.preventDefault();
      m.show();
    });
    
        if (!!window.EventSource) {
           source = new EventSource("/events");
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
               console.log("Event message   ",new Date().getTime().toString());
               setWebsock();
               source.close();
            }, false);
        }     
    
}



function setBulb(value)
{
  var bulb = document.getElementById("bulb");
  bulb.src = srcs[value];
  var btn = document.getElementById("btn");
  btn.innerHTML = txts[value];
  btn.classList.remove(classes[value? 0:1]);
  btn.classList.add(classes[value]);
}


function setHumiTemp(objct)
{
    var temp = document.getElementById("temp");
    temp.innerHTML = objct.temperature;
    var humi = document.getElementById("humi");
    humi.innerHTML = objct.humidity;  
}

//function sendCommand(cmd, kwargs)
//{
//    var commandtosend = {};
//    commandtosend.command = cmd;
//        for (var k in kwargs) {
//        if (kwargs.hasOwnProperty(k)) {
//           commandtosend[k] = kwargs[k];
//        }
//    }
//    websock.send(JSON.stringify(commandtosend));
//}


function toggleState()
{
  websock.send("{\"command\":\"toggle\"}");
}

function reloadPage()
{
  window.location.reload(true); 
  //var btn = document.getElementById("btn");
}


function setWebsock()
{
  websock = new WebSocket("ws://" + window.location.hostname +    ":"+window.location.port+        "/ws");
  lastTimeStamp = Date.now();
  websock.onopen  = function(evt) {var interval = setInterval(test, 40000);};
  websock.onclose = function(evt) {};
  websock.onerror = function(evt) {console.log(evt);};
  websock.onmessage = function(evt)
  {
    var obj = JSON.parse(evt.data);
  if (obj.command === "state") 
  {
    document.getElementById("loading-img").style.display = "none";
    setBulb(obj.state);
    setHumiTemp(obj);
  }
  else if (obj.command === "change") 
  {
    setBulb(obj.state);
  }
  else if (obj.command === "password")
  {
    websock.send("{\"command\":\"password\",\"password\":\""+password+"\"}");
  }
  else if (obj.command === "button")
  {
    alert("Button pressed !");
  }
  else if (obj.command === "authorized")
  {
    websock.send("{\"command\":\"getconf\"}");
    var commandtosend = {};
    //commandtosend.command = "pinlist";
    commandtosend.command = "getstate";
    websock.send(JSON.stringify(commandtosend));
  }
  else if (obj.command === "configfile")
  {
    var hostnm = document.getElementById("hostnm");
    hostnm.innerHTML = obj.hostnm;
  }
  else if (obj.command === "nop")
    {
    lastTimeStamp = Date.now();
    setHumiTemp(obj);
    }
  }
}
