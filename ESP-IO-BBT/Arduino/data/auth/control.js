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
var count=0;

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
               console.log("Event message   ",new Date().getTime().toString());
               setWebsock();
               source.close();
            }, false);
        }     
    
}

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

