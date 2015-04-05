function callback(name, data) {
  if (!data) data = '';
  window.location.href = 'skp:' + name + '@' + data;
}

function size_changed() {
  var w = $( 'body' ).width();
  var h = $( 'body' ).height();
  var data = '['+w+','+h+']';
  callback('size_changed', data);
}

function init() {
  callback('init');
}

function create_slider(name, starting_value, min, max, step) {
  CARPE.Sliders.make(name, {
    'orientation': 'horizontal',
    'targets': 'display2 display3',
    'size': 200,
    'position': 100,
    'stops': 10,
    'source': 'display2',
    'decimals': 2,
    'zerofill': 'yes',
    'min': min,
    'max': max,
    'from': 0,
    'to': 1,
    'name': name });
}

function destroy_slider() {
}
$(document).ready( function() {
  window.setTimeout(init, 0);
  window.setTimeout(size_changed, 0);
});
