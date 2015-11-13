var messageQueue = [];
var MAX_ERRORS = 5;
var errorCount = 0;
var v = 56;

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS Ready!');

  // Construct a dictionary
  var dict = {
    'time': new Date().getTime()/1000,
    'glucose': 56
  };

  // Send a string to Pebble
  Pebble.sendAppMessage(dict,
    function(e) {
      console.log('Send successful.');
    },
    function(e) {
      console.log('Send failed!');
    }
  );
  for (var i=36; i>=0; i--) {
    v = Math.max(Math.min(330, v + Math.random()*30-15), 20);
    messageQueue.push({
      'time': new Date().getTime()/1000-i*5*60,
      'glucose': v
    });
  }
  sendNextMessage();
});

function sendNextMessage() {
  if (messageQueue.length > 0) {
    Pebble.sendAppMessage(messageQueue[0], appMessageAck, appMessageNack);
    // console.log("Sent message to Pebble! " + messageQueue.length + ': ' + JSON.stringify(messageQueue[0]));
  }
}

function appMessageAck(e) {
  // console.log("Message accepted by Pebble!");
  messageQueue.shift();
  sendNextMessage();
}

function appMessageNack(e) {
  console.warn("Message rejected by Pebble! " + e.error);
  if (e && e.data && e.data.transactionId) {
    // console.log("Rejected message id: " + e.data.transactionId);
  }
  if (errorCount >= MAX_ERRORS) {
    messageQueue.shift();
  }
  else {
    errorCount++;
    console.log("Retrying, " + errorCount);
  }
  sendNextMessage();
}

setInterval(function() {
  messageQueue.push({
    'time': new Date().getTime()/1000,
    'glucose': Math.max(Math.min(330, v + Math.random()*30-15), 20)  
  });
  sendNextMessage();
}, 60000);

