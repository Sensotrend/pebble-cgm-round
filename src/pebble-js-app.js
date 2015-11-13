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

var t = new Date().getTime()/1000-3*60*60;
var v = 56;
setInterval(function() {
  v = Math.max(Math.min(330, v + Math.random()*30-15), 20);
  Pebble.sendAppMessage({
    'time': t,
    'glucose': v
  });
  t += 300;
}, 2000);

