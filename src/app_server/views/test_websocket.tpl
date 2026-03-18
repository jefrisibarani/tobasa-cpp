{% extends "layouts/layout_tests.tpl" %}

{% block pageContent %}
          <div class="container">
             <h2>WebSocket Test Page</h2>
             <div class="card mb-1">
                <div class="card-body">
                  <div class="mb-2 d-flex">
                    <span class="me-1">Server:</span>
                    <input type="text" class="form-control form-control-sm me-1" 
                       style="width:290px;" id="txtEndpoint" type="text"
                       value="wss://localhost:8085/websocket_ep">
                    </input>
                    <span class="me-1">Username:</span>
                    <input type="text" class="form-control form-control-sm me-1" 
                       style="width:90px;" id="txtUSername" type="text"
                       value="User001">
                    </input>
                    <button type="button" class="btn btn-sm btn-outline-secondary ms-auto me-1" id="btnConnect" >Connect</button>
                  </div>
                  <div id="eventOutput" class="mb-2" style="padding:5px;height:180px;overflow: auto; border-style: solid;border-width:1px;border-color: #cfcfcf ;border-radius:10px;"></div>
                  <div class="mb-1">Chat messages</div>
                  <div id="msgOutput"   class="mb-2" style="padding:5px;height:200px;overflow: auto; border-style: solid;border-width:1px;border-color: #cfcfcf ;border-radius:10px;"></div>
                  <textarea id="msgInput" class="mt-2 mb-2 form-control" rows="3"></textarea>
                  <div class="d-flex">
                     <span class="me-1">Send To:</span>
                     <select id="user_list_select">
                       <!--option value="user_1">User 1</option-->
                     </select>
                     <button type="button" class="btn btn-sm btn-outline-secondary ms-auto me-1 mb-2" id="btnSend" disabled>Send</button>
                  </div>
                </div>
             </div>
          </div>
{% endblock %}

{% block pageVendorJS %}
    {% if dataTestProtobuf %}
    <!--script src="https://cdn.jsdelivr.net/npm/protobufjs@7.3.2/dist/minimal/protobuf.min.js"></script-->
    <!--script src="{{ pageBaseUrl }}/js/chat_types_samples.js"></script-->
     {% include "test_websocket_js.tpl" %}
    {% endif %}
    <script src="{{ pageBaseUrl }}/js/generate_pushid.js" crossorigin="anonymous"></script>
{% endblock %}

{% block pageJS %}
 <script>
    // http://www.websocket.org/echo.html
    // ----------------------------------------------
    const btnConnect  = document.getElementById("btnConnect");
    const btnSend     = document.getElementById("btnSend");
    const msgOutput   = document.getElementById("msgOutput");
    const eventOutput = document.getElementById("eventOutput");
    const msgInput    = document.getElementById("msgInput");
    const txtEndpoint = document.getElementById("txtEndpoint");
    const txtUSername = document.getElementById("txtUSername");
    const userSelect  = document.getElementById("user_list_select");
    const endpoint    = txtEndpoint.value.trim();

    btnConnect.addEventListener("click", onConnect);
    btnSend.addEventListener("click", onSend);
    // ----------------------------------------------
    let myUserId   = null;
    let myConnId   = null;
    let myUSerName = txtUSername.value;

    {% if dataTestProtobuf %}
    const msg = protobuf.roots.default.chatmsg; 
    {% endif %}
    // ----------------------------------------------
    let websocket = null;

    function onConnect() {
      myUSerName = txtUSername.value;
      websocket  = new WebSocket(endpoint);
      
      {% if dataTestProtobuf %}
      websocket.binaryType = "arraybuffer";
      {% endif %}

      websocket.onopen = (event) => {
        appendEventMsg(`[EVT ] Connected to WebSocket server at ${endpoint}`);
        btnSend.disabled = false;
        doSend(`SYS_JOIN_CHAT`, "", "server001", "SYS");
        // we can not do this here, because we haven't got userId from server
        //doSend('SYS_GET_USER_LIST', "", "server001", "SYS");
      };

      websocket.onmessage = (event) => {
        var timestamp;
        var messageId;
        var messageType;
        var senderId;
        var senderName;
        var receiverId;
        var messageCmd;
        var content;

        {% if dataTestProtobuf %}

        // Decode the binary buffer using protobufjs
        try {
          let buffer  = getBuffer(event);
          const msgIn = msg.ChatMessage.decode(buffer);

          timestamp   = msgIn.timestamp;
          messageId   = msgIn.id;
          messageType = msg.ChatMessageType[msgIn.messageType];
          senderId    = msgIn.senderId;
          senderName  = msgIn.senderName;
          receiverId  = msgIn.receiverId;
          messageCmd  = msgIn.messageCmd;
          content     = msgIn.content;
        } catch (error) {
          appendEventMsg(`[ERR ] Failed to decode message: ${error}`);
          return;
        }

        {% else %}

          // Syntax: TBSMSG|{timestamp}|{message_id}|{message_type}|{sender_id}|{sender_name}|{receiver_id}|{messageCmd}|{content}
          // e.g   : TBSMSG|1738328617569|-OHwSQ8XFWvkgCqOFzKd|MSG_TYPE_SYS|server001|server001|VziZC9hcD1|SYS_RET_USER_LIST|[{"userId":"VziZC9hcD1","userName":"Smith"}]'
          // message_type : MSG_TYPE_TEXT, MSG_TYPE_STICKER, MSG_TYPE_NOTIFICATION, MSG_TYPE_SYS 
          // message_cmd  : APP_TEXT, SYS_KILL_ME_NOW, SYS_JOIN_CHAT, SYS_GET_USER_LIST, SYS_RET_USER_LIST, SYS_RET_USER_INFO
          const parts = event.data.split('|');
          if (parts.length == 9) {
            timestamp   = parts[1];
            messageId   = parts[2];
            messageType = parts[3];
            senderId    = parts[4];
            senderName  = parts[5];
            receiverId  = parts[6];
            messageCmd  = parts[7];
            content     = parts[8];
          } else {
            appendEventMsg(`[ERR ] Invalid message format: ${event.data}`);
            return;
          }

        {% endif %}
        
        if (messageCmd == null) {
          appendEventMsg(`[ERR ] Invalid message command`);
          return;
        }

        if (messageType === 'MSG_TYPE_TEXT' && messageCmd === 'APP_TEXT') {
          const lines = content.split('\n');
          // Process each line
          lines.forEach((line, index) => {
            appendMessage(`[${senderName}] ${line}`);
          });
        }
        else if (messageType === 'MSG_TYPE_SYS') {
          processSysMessage(messageCmd, content );
        }
      };

      websocket.onclose = (event) => {
        appendEventMsg("[EVT ] Disconnected from WebSocket server.");
        btnSend.disabled = true;
      };

      websocket.onerror = (error) => {
        appendEventMsg(`[ERR ] ${(error.message || 'An unknown error occurred.')}`);
      };

      return websocket;
    }

    function onSend() {
      const receiverId = userSelect.value;
      const message    = msgInput.value.trim();
      // Do not send SYS messages from Input Text Box
      if (message.startsWith('SYS_')) {
        alert('Invalid message');
        return;
      }

      message && doSend("APP_TEXT", message, receiverId, "TEXT" );
      msgInput.value = ''; // clear input
      msgInput.focus();
    }

    function doSend(messageCmd, content, receiverId, messageType) {
      if (websocket && websocket.readyState === WebSocket.OPEN && messageCmd) {

        {% if dataTestProtobuf %}

        // Protobuf message
        var msgType;
        if (messageType === "TEXT")
          msgType = msg.ChatMessageType.MSG_TYPE_TEXT;
        if (messageType === "STICKER")
          msgType = msg.ChatMessageType.MSG_TYPE_STICKER;
        if (messageType === "NOTIFICATION")
          msgType = msg.ChatMessageType.MSG_TYPE_NOTIFICATION;
        if (messageType === "SYS")
          msgType = msg.ChatMessageType.MSG_TYPE_SYS;

        let chatMsg = msg.ChatMessage.create({
          id : newMessageId(),
          content: content,
          senderId: myUserId,
          senderName: myUSerName,
          receiverId: receiverId,
          messageType: msgType,
          messageCmd: messageCmd,
          timestamp: newMessageTimestamp(), //  get unix timestamp in ms
        });

        // Serialize the message to a binary format
        const buffer = msg.ChatMessage.encode(chatMsg).finish();
        websocket.send(buffer);

        {% else %}

        // Raw text message
        // Syntax: TBSMSG|{timestamp}|{message_id}|{message_type}|{sender_id}|{sender_name}|{receiver_id}|{messageCmd}|{content}
        // e.g   : TBSMSG|1738328617569|-OHwSQ8XFWvkgCqOFzKd|MSG_TYPE_SYS|server001|server001|VziZC9hcD1|SYS_RET_USER_LIST|[{"userId":"VziZC9hcD1","userName":"Smith"}]'
        // message_type : MSG_TYPE_TEXT, MSG_TYPE_STICKER, MSG_TYPE_NOTIFICATION, MSG_TYPE_SYS 
        // message_cmd  : APP_TEXT, SYS_KILL_ME_NOW, SYS_JOIN_CHAT, SYS_GET_USER_LIST, SYS_RET_USER_LIST, SYS_RET_USER_INFO
        var buffer = "TBSMSG" + "|" + newMessageTimestamp();
        buffer += "|" + newMessageId();
        buffer += "|" + "MSG_TYPE_" + messageType;
        buffer += "|" + myUserId + "|" + myUSerName;
        buffer += "|" + receiverId;
        buffer += "|" + messageCmd + "|" + content;

        websocket.send(buffer);

        {% endif %}

        if (messageType === 'SYS')
          appendEventMsg(`[SENT] ${content}`);
        else
          appendMessage(`[${myUSerName}] ${content}`);
      }
      else
        appendEventMsg(`[ERR ] WebSocket is not connected or content is empty.`);
    }

    // Append messages to the message display area
    function appendMessage(val) {
       //msgOutput.insertAdjacentHTML("afterbegin", `<p style="margin-bottom:0px">${val}</p>`);
       const el = document.createElement('div');

       if (val.includes("[RECV]"))
          el.style.color = "blue";
       else if (val.includes("[ERR ]"))
          el.style.color = "red"; 
       else if (val.includes("[EVT ]"))
          el.style.color = "brown"; 
       else // [SENT]
          el.style.color = "green";

       el.textContent = val;
       msgOutput.appendChild(el);
       msgOutput.scrollTop = msgOutput.scrollHeight; // Auto-scroll to the bottom
    }
    
    // Append event messages to the eventOutput display area
    function appendEventMsg(val) {
       const el = document.createElement('div');

       if (val.includes("[RECV]"))
          el.style.color = "blue";
       else if (val.includes("[ERR ]"))
          el.style.color = "red"; 
       else if (val.includes("[EVT ]"))
          el.style.color = "brown"; 
       else // [SENT]
          el.style.color = "green";

       el.textContent = val;
       eventOutput.appendChild(el);
       eventOutput.scrollTop = msgOutput.scrollHeight; // Auto-scroll to the bottom
    }

    function getBuffer(event) {
      if (typeof event.data === 'string') {
        var binaryString = event.data;
        var buffer = new Uint8Array(binaryString.length);
        for (var i = 0; i < binaryString.length; i++) {
          buffer[i] = binaryString.charCodeAt(i);
        }
        return buffer;

      } else if (event.data instanceof ArrayBuffer) {
        // If event.data is already in binary format
        var buffer = new Uint8Array(event.data);
        return buffer;
      }
      else {
        throw "Invalid data type";
      }
      return null;
    }

    function processSysMessage(messageCmd, content)
    {
      if (messageCmd == 'SYS_RET_USER_INFO') {

        const userInfo = JSON.parse(content);
        myUserId = userInfo.userId;
        myConnId = userInfo.connId;

        appendEventMsg(`[RECV] ${messageCmd}`);
        appendEventMsg(`[INFO] User ID: ${myUserId}, Connection ID: ${myConnId}`);

        return;
      }
      else if (messageCmd == 'SYS_RET_USER_LIST') {
        
        appendEventMsg(`[RECV] ${messageCmd}`);

        // Update user list combo box
        const userList       = JSON.parse(content);
        userSelect.innerHTML = ''; // Clear existing options

        // first item is ALL USER
        const option0 = document.createElement("option");
        option0.value = 'all_user';
        option0.textContent = 'ALL USER';
        userSelect.appendChild(option0);

        userList.forEach(user => {
          const option = document.createElement("option");
          option.value = user.userId;
          if (user.userId == myUserId)
            option.textContent = '[Me] ' + user.userName;
          else
            option.textContent = user.userName;

          userSelect.appendChild(option);
        });

        return;
      }

      return;
    }

    function newMessageId() {
      return generatePushID();
    }

    function newMessageTimestamp() {
      let now = new Date();
      return now.getTime().toString(); //  get unix timestamp in ms
    }
  </script>

{% endblock %}