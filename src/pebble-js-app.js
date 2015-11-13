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
});

setTimeout(function() {
  Pebble.sendAppMessage({
    'time': new Date().getTime()/1000,
    'glucose': 30
  });
}, 20000);

