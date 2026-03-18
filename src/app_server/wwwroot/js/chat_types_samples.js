/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
"use strict";

var $protobuf = window.protobuf; // var $protobuf = require("protobufjs/minimal");

// Common aliases
var $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
var $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

$root.chatmsg = (function() {

    /**
     * Namespace chatmsg.
     * @exports chatmsg
     * @namespace
     */
    var chatmsg = {};

    /**
     * ChatMessageType enum.
     * @name chatmsg.ChatMessageType
     * @enum {number}
     * @property {number} MSG_TYPE_TEXT=0 MSG_TYPE_TEXT value
     * @property {number} MSG_TYPE_STICKER=1 MSG_TYPE_STICKER value
     * @property {number} MSG_TYPE_NOTIFICATION=2 MSG_TYPE_NOTIFICATION value
     * @property {number} MSG_TYPE_SYS=3 MSG_TYPE_SYS value
     */
    chatmsg.ChatMessageType = (function() {
        var valuesById = {}, values = Object.create(valuesById);
        values[valuesById[0] = "MSG_TYPE_TEXT"] = 0;
        values[valuesById[1] = "MSG_TYPE_STICKER"] = 1;
        values[valuesById[2] = "MSG_TYPE_NOTIFICATION"] = 2;
        values[valuesById[3] = "MSG_TYPE_SYS"] = 3;
        return values;
    })();

    chatmsg.ChatMessage = (function() {

        /**
         * Properties of a ChatMessage.
         * @memberof chatmsg
         * @interface IChatMessage
         * @property {string|null} [id] ChatMessage id
         * @property {string|null} [content] ChatMessage content
         * @property {string|null} [senderId] ChatMessage senderId
         * @property {string|null} [senderName] ChatMessage senderName
         * @property {string|null} [receiverId] ChatMessage receiverId
         * @property {string|null} [timestamp] ChatMessage timestamp
         * @property {chatmsg.ChatMessageType|null} [messageType] ChatMessage messageType
         * @property {string|null} [messageCmd] ChatMessage messageCmd
         */

        /**
         * Constructs a new ChatMessage.
         * @memberof chatmsg
         * @classdesc Represents a ChatMessage.
         * @implements IChatMessage
         * @constructor
         * @param {chatmsg.IChatMessage=} [properties] Properties to set
         */
        function ChatMessage(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * ChatMessage id.
         * @member {string} id
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.id = "";

        /**
         * ChatMessage content.
         * @member {string} content
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.content = "";

        /**
         * ChatMessage senderId.
         * @member {string} senderId
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.senderId = "";

        /**
         * ChatMessage senderName.
         * @member {string} senderName
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.senderName = "";

        /**
         * ChatMessage receiverId.
         * @member {string} receiverId
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.receiverId = "";

        /**
         * ChatMessage timestamp.
         * @member {string} timestamp
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.timestamp = "";

        /**
         * ChatMessage messageType.
         * @member {chatmsg.ChatMessageType} messageType
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.messageType = 0;

        /**
         * ChatMessage messageCmd.
         * @member {string} messageCmd
         * @memberof chatmsg.ChatMessage
         * @instance
         */
        ChatMessage.prototype.messageCmd = "";

        /**
         * Creates a new ChatMessage instance using the specified properties.
         * @function create
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {chatmsg.IChatMessage=} [properties] Properties to set
         * @returns {chatmsg.ChatMessage} ChatMessage instance
         */
        ChatMessage.create = function create(properties) {
            return new ChatMessage(properties);
        };

        /**
         * Encodes the specified ChatMessage message. Does not implicitly {@link chatmsg.ChatMessage.verify|verify} messages.
         * @function encode
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {chatmsg.IChatMessage} message ChatMessage message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        ChatMessage.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.id != null && Object.hasOwnProperty.call(message, "id"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.id);
            if (message.content != null && Object.hasOwnProperty.call(message, "content"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.content);
            if (message.senderId != null && Object.hasOwnProperty.call(message, "senderId"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.senderId);
            if (message.senderName != null && Object.hasOwnProperty.call(message, "senderName"))
                writer.uint32(/* id 4, wireType 2 =*/34).string(message.senderName);
            if (message.receiverId != null && Object.hasOwnProperty.call(message, "receiverId"))
                writer.uint32(/* id 5, wireType 2 =*/42).string(message.receiverId);
            if (message.timestamp != null && Object.hasOwnProperty.call(message, "timestamp"))
                writer.uint32(/* id 6, wireType 2 =*/50).string(message.timestamp);
            if (message.messageType != null && Object.hasOwnProperty.call(message, "messageType"))
                writer.uint32(/* id 7, wireType 0 =*/56).int32(message.messageType);
            if (message.messageCmd != null && Object.hasOwnProperty.call(message, "messageCmd"))
                writer.uint32(/* id 8, wireType 2 =*/66).string(message.messageCmd);
            return writer;
        };

        /**
         * Encodes the specified ChatMessage message, length delimited. Does not implicitly {@link chatmsg.ChatMessage.verify|verify} messages.
         * @function encodeDelimited
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {chatmsg.IChatMessage} message ChatMessage message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        ChatMessage.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a ChatMessage message from the specified reader or buffer.
         * @function decode
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {chatmsg.ChatMessage} ChatMessage
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        ChatMessage.decode = function decode(reader, length) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.chatmsg.ChatMessage();
            while (reader.pos < end) {
                var tag = reader.uint32();
                switch (tag >>> 3) {
                case 1: {
                        message.id = reader.string();
                        break;
                    }
                case 2: {
                        message.content = reader.string();
                        break;
                    }
                case 3: {
                        message.senderId = reader.string();
                        break;
                    }
                case 4: {
                        message.senderName = reader.string();
                        break;
                    }
                case 5: {
                        message.receiverId = reader.string();
                        break;
                    }
                case 6: {
                        message.timestamp = reader.string();
                        break;
                    }
                case 7: {
                        message.messageType = reader.int32();
                        break;
                    }
                case 8: {
                        message.messageCmd = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a ChatMessage message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {chatmsg.ChatMessage} ChatMessage
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        ChatMessage.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a ChatMessage message.
         * @function verify
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        ChatMessage.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.id != null && message.hasOwnProperty("id"))
                if (!$util.isString(message.id))
                    return "id: string expected";
            if (message.content != null && message.hasOwnProperty("content"))
                if (!$util.isString(message.content))
                    return "content: string expected";
            if (message.senderId != null && message.hasOwnProperty("senderId"))
                if (!$util.isString(message.senderId))
                    return "senderId: string expected";
            if (message.senderName != null && message.hasOwnProperty("senderName"))
                if (!$util.isString(message.senderName))
                    return "senderName: string expected";
            if (message.receiverId != null && message.hasOwnProperty("receiverId"))
                if (!$util.isString(message.receiverId))
                    return "receiverId: string expected";
            if (message.timestamp != null && message.hasOwnProperty("timestamp"))
                if (!$util.isString(message.timestamp))
                    return "timestamp: string expected";
            if (message.messageType != null && message.hasOwnProperty("messageType"))
                switch (message.messageType) {
                default:
                    return "messageType: enum value expected";
                case 0:
                case 1:
                case 2:
                case 3:
                    break;
                }
            if (message.messageCmd != null && message.hasOwnProperty("messageCmd"))
                if (!$util.isString(message.messageCmd))
                    return "messageCmd: string expected";
            return null;
        };

        /**
         * Creates a ChatMessage message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {chatmsg.ChatMessage} ChatMessage
         */
        ChatMessage.fromObject = function fromObject(object) {
            if (object instanceof $root.chatmsg.ChatMessage)
                return object;
            var message = new $root.chatmsg.ChatMessage();
            if (object.id != null)
                message.id = String(object.id);
            if (object.content != null)
                message.content = String(object.content);
            if (object.senderId != null)
                message.senderId = String(object.senderId);
            if (object.senderName != null)
                message.senderName = String(object.senderName);
            if (object.receiverId != null)
                message.receiverId = String(object.receiverId);
            if (object.timestamp != null)
                message.timestamp = String(object.timestamp);
            switch (object.messageType) {
            default:
                if (typeof object.messageType === "number") {
                    message.messageType = object.messageType;
                    break;
                }
                break;
            case "MSG_TYPE_TEXT":
            case 0:
                message.messageType = 0;
                break;
            case "MSG_TYPE_STICKER":
            case 1:
                message.messageType = 1;
                break;
            case "MSG_TYPE_NOTIFICATION":
            case 2:
                message.messageType = 2;
                break;
            case "MSG_TYPE_SYS":
            case 3:
                message.messageType = 3;
                break;
            }
            if (object.messageCmd != null)
                message.messageCmd = String(object.messageCmd);
            return message;
        };

        /**
         * Creates a plain object from a ChatMessage message. Also converts values to other types if specified.
         * @function toObject
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {chatmsg.ChatMessage} message ChatMessage
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        ChatMessage.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.id = "";
                object.content = "";
                object.senderId = "";
                object.senderName = "";
                object.receiverId = "";
                object.timestamp = "";
                object.messageType = options.enums === String ? "MSG_TYPE_TEXT" : 0;
                object.messageCmd = "";
            }
            if (message.id != null && message.hasOwnProperty("id"))
                object.id = message.id;
            if (message.content != null && message.hasOwnProperty("content"))
                object.content = message.content;
            if (message.senderId != null && message.hasOwnProperty("senderId"))
                object.senderId = message.senderId;
            if (message.senderName != null && message.hasOwnProperty("senderName"))
                object.senderName = message.senderName;
            if (message.receiverId != null && message.hasOwnProperty("receiverId"))
                object.receiverId = message.receiverId;
            if (message.timestamp != null && message.hasOwnProperty("timestamp"))
                object.timestamp = message.timestamp;
            if (message.messageType != null && message.hasOwnProperty("messageType"))
                object.messageType = options.enums === String ? $root.chatmsg.ChatMessageType[message.messageType] === undefined ? message.messageType : $root.chatmsg.ChatMessageType[message.messageType] : message.messageType;
            if (message.messageCmd != null && message.hasOwnProperty("messageCmd"))
                object.messageCmd = message.messageCmd;
            return object;
        };

        /**
         * Converts this ChatMessage to JSON.
         * @function toJSON
         * @memberof chatmsg.ChatMessage
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        ChatMessage.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for ChatMessage
         * @function getTypeUrl
         * @memberof chatmsg.ChatMessage
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        ChatMessage.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/chatmsg.ChatMessage";
        };

        return ChatMessage;
    })();

    return chatmsg;
})();

//module.exports = $root;
