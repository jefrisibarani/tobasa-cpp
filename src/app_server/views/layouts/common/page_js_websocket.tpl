    <style>
      .notification_icon {
        position: relative !important;
        cursor: pointer;
        transition: all 0.3s ease;
        display: inline-flex !important;
        align-items: center;
        justify-content: center;
        color: inherit;
      }

      .notification_icon:hover {
        color: #ffc107;
        transform: scale(1.1);
      }

      .notification_icon.has-unread {
        color: #ffc107 !important;
      }

      .notification_badge {
        display: inline-block;
        margin-left: 3px;
        background-color: #ff1744 !important;
        color: white !important;
        border-radius: 50%;
        min-width: 20px;
        height: 20px;
        line-height: 20px;
        text-align: center;
        font-size: 11px;
        font-weight: 900;
        padding: 0 2px;
        vertical-align: middle;
        box-shadow: 0 2px 4px rgba(0,0,0,0.3);
      }
    </style>

    <script type="text/javascript"> <!-- JS ALERT -->

      class EventMessage {
         constructor(type = '', message = '', data = {}) {
            this.type = type;
            this.message = message;
            this.data = data;
         }

         toJson() {
            const result = {
               type: this.type,
               message: this.message
            };

            if (this.data !== null &&
                typeof this.data === 'object' &&
                Object.keys(this.data).length > 0) {
               result.data = this.data;
            }

            return result;
         }

         static fromJson(value) {
            const payload = typeof value === 'string' ? JSON.parse(value) : value;
            if (!payload || typeof payload !== 'object') {
               throw new TypeError('EventMessage payload must be an object');
            }

            return new EventMessage(
               typeof payload.type === 'string' ? payload.type : '',
               typeof payload.message === 'string' ? payload.message : '',
               payload.data ?? {}
            );
         }
      }

      function handleConnectedEvent(eventMessage)
      {
         if (eventMessage.data && eventMessage.data.wsConnIdentity !== undefined) {
            TBS.log('[EVT ] WebSocket connection ID: ' + eventMessage.data.wsConnIdentity);
            TBS.wsConnIdentity = eventMessage.data.wsConnIdentity;
         }
      }

      function handleDicomExportCompletedEvent(eventMessage)
      {
         const downloadId = eventMessage.data && eventMessage.data.downloadId;
         if (downloadId) {
            TBS.log('[EVT ] DICOM export completed: ' + downloadId);
         }

         TBS.alert.info(
            eventMessage.message || 'DICOM files exported successfully.',
            'Toast',
            ''
         );
      }

      // Notification system
      let notificationsList = [];

      function handleNotificationEvent(eventMessage)
      {
         const notification = {
            id: eventMessage.data?.id || Date.now(),
            type: eventMessage.data?.type || 'info',
            title: eventMessage.data?.title || eventMessage.message,
            content: eventMessage.data?.content || '',
            timestamp: eventMessage.data?.timestamp || new Date().toISOString(),
            action: eventMessage.data?.action || null,
            read: false
         };

         notificationsList.unshift(notification);
         updateNotificationBadge();
         TBS.log('[NOTIF] Received: ' + notification.title);
      }

      function updateNotificationBadge()
      {
         const unreadCount = notificationsList.filter(n => !n.read).length;
         const icon = document.getElementById('notification_icon');
         
         if (!icon) {
            TBS.log('[WARN] notification_icon element not found');
            return;
         }
         
         // Remove existing badge if any
         let existingBadge = icon.querySelector('.notification_badge');
         if (existingBadge) {
            existingBadge.remove();
         }
         
         if (unreadCount > 0) {
            icon.classList.add('has-unread');
            // Create and add badge element
            const badge = document.createElement('span');
            badge.className = 'notification_badge';
            badge.textContent = unreadCount > 99 ? '99+' : unreadCount;
            icon.appendChild(badge);
            TBS.log('[NOTIF] Unread count: ' + unreadCount + ', badge added to DOM');
         } else {
            icon.classList.remove('has-unread');
            TBS.log('[NOTIF] No unread notifications, has-unread class removed');
         }
      }

      function showNotificationDialog()
      {
         notificationsList.forEach(n => n.read = true);
         updateNotificationBadge();

         let content = '<div style="max-height: 500px; overflow-y: auto; padding: 10px;">';
         
         if (notificationsList.length === 0) {
            content += '<div class="text-muted text-center py-4"><p>📭 No notifications</p></div>';
         } else {
            notificationsList.forEach(notif => {
               const iconMap = {
                  'info': '📋',
                  'warning': '⚠️',
                  'error': '❌',
                  'success': '✅'
               };
               const icon = iconMap[notif.type] || '📋';
               const time = new Date(notif.timestamp).toLocaleString();
               
               let actionHtml = '';
               if (notif.action && notif.action.actionType === 'button' && notif.action.actionLink) {
                  actionHtml = `<div style="margin-top:10px;"><a href="${notif.action.actionLink}" class="btn btn-sm btn-primary" target="_blank">Download</a></div>`;
               }
               
               content += `
                  <div style="padding:12px; margin-bottom:10px; border-left:4px solid #ffc107; background:#f8f9fa; border-radius:4px;">
                     <div style="display:flex; align-items:start; gap:10px;">
                        <span style="font-size:20px;">${icon}</span>
                        <div style="flex:1;">
                           <h6 style="margin:0 0 5px 0; font-weight:600; color:#333;">${notif.title}</h6>
                           ${notif.content ? `<p style="margin:0 0 5px 0; font-size:13px; color:#666;">${notif.content}</p>` : ''}
                           <small style="color:#999; font-size:12px;">${time}</small>
                           ${actionHtml}
                        </div>
                     </div>
                  </div>
               `;
            });
         }
         
         content += '</div>';

         bootbox.dialog({
            title: '🔔 Notifications (' + notificationsList.length + ')',
            message: content,
            backdrop: true,
            size: 'large',
            buttons: {
               close: {
                  label: 'Close',
                  className: 'btn-primary'
               }
            }
         });
      }

      let socket = null;
      let appSocketUrl = TBS.urlWebsocketEndpoint();
      window.addEventListener('DOMContentLoaded', event => {
         try {
            // Initialize notification icon
            const notificationIcon = document.getElementById('notification_icon');
            TBS.log('[NOTIF] Icon element found: ' + (notificationIcon ? 'YES' : 'NO'));
            
            if (notificationIcon) {
               notificationIcon.addEventListener('click', showNotificationDialog);
               
               // Debug: Log computed styles
               const computed = window.getComputedStyle(notificationIcon);
               TBS.log('[NOTIF] Icon computed - display: ' + computed.display + ', visibility: ' + computed.visibility + ', position: ' + computed.position);
               TBS.log('[NOTIF] Icon computed - top: ' + computed.top + ', right: ' + computed.right + ', zIndex: ' + computed.zIndex);
               TBS.log('[NOTIF] Icon size - width: ' + computed.width + ', height: ' + computed.height);
               
               // Debug: Log bounding rect
               const rect = notificationIcon.getBoundingClientRect();
               TBS.log('[NOTIF] Icon rect - top: ' + rect.top + ', right: ' + rect.right + ', width: ' + rect.width + ', height: ' + rect.height);
            }
            updateNotificationBadge();

            if (typeof appSocketUrl !== 'string' || !appSocketUrl.trim()) {
               TBS.log('[WARN] WebSocket endpoint is not configured.');
               return;
            }

            // WebSocket construction can throw for an invalid URL or security policy.
            socket = new WebSocket(appSocketUrl);

            socket.addEventListener('open', () => {
               TBS.log("[EVT ] Connected to WebSocket server at " + appSocketUrl);
            });

            socket.addEventListener('message', (event) => {
               try {
                  if (typeof event.data !== 'string') {
                     TBS.log('[WARN] Ignoring unsupported WebSocket message type.');
                     return;
                  }

                  const rawMessage = event.data.trim();
                  if (!rawMessage) {
                     return;
                  }

                  let payload;
                  try {
                     payload = JSON.parse(rawMessage);
                  } catch (parseError) {
                     // The server sends a plain-text welcome message on connect.
                     TBS.alert.info(rawMessage, 'Toast', '');
                     return;
                  }

                  const eventMessage = EventMessage.fromJson(payload);
                  if (eventMessage.type === 'websocket.connected') {
                     handleConnectedEvent(eventMessage);
                     return;
                  }
                  if (eventMessage.type === 'dicom.export.completed') {
                     handleDicomExportCompletedEvent(eventMessage);
                     return;
                  }
                  if (eventMessage.type === 'notification') {
                     handleNotificationEvent(eventMessage);
                     return;
                  }

                  if (!eventMessage.message) {
                     TBS.log('[WARN] WebSocket JSON message has no message field.');
                     return;
                  }

                  const type = eventMessage.type.toLowerCase();

                  if (type === 'error' || type === 'failure') {
                     TBS.alert.error(eventMessage.message, 'Toast', '');
                  } else {
                     TBS.alert.info(eventMessage.message, 'Toast', '');
                  }
               } catch (error) {
                  TBS.log('[ERR ] Failed to handle WebSocket message: ' +
                     (error.message || 'An unknown error occurred.'));
               }
            });

            socket.addEventListener('close', () => {
               TBS.log("[EVT ] Disconnected from WebSocket server.");
               socket = null;
            });

            socket.addEventListener('error', (error) => {
               TBS.log("[ERR ] " + (error.message || "An unknown error occurred."));
            });
         } catch (error) {
            socket = null;
            TBS.log('[ERR ] Failed to initialize WebSocket: ' +
               (error.message || 'An unknown error occurred.'));
         }
      });
    </script> <!-- JS WEBSOCKET -->