var sliders = {};

function callback(v_name, v_data) {
  if (!v_data) v_data = '';
  window.location.href = 'skp:' + v_name + '@' + v_data;
}

function size_changed() {
  var w = $( 'body' ).width();
  var h = $( 'body' ).height();
  var data = '['+w+','+h+']';
  callback('size_changed', data);
}

function add_slider(v_name, v_default_value, v_min, v_max, v_step) {
  if (v_name in sliders) return false;
  var text = "<tr id=\"tcrs-" + v_name + "\">"
  text += "<td><div class=\"controller_name\">" + v_name + "</div></td>"
  text += "<td><div class=\"spacing_tab\"></div></td>"
  text += "<td><input class=\"controller_value\" type=\"text\" tabindex=\"-1\" id=\"icrs-" + v_name + "\" /></td>"
  text += "<td><input class=\"controller_value\" style=\"display: none;\" type=\"text\" tabindex=\"-1\" readonly id=\"lcrs-" + v_name + "\" /></td>"
  text += "<td><div class=\"spacing_tab\"></div></td>"
  text += "<td><div id=\"scrs-" + v_name + "\"></div></td>"
  text += "</tr>"
  $( '#table-crs' ).append(text);
  var slider = new dhtmlXSlider({
    parent: "scrs-" + v_name,
    size:   164,
    step:   v_step,
    min:    v_min,
    max:    v_max,
    value:  v_default_value,
    linkTo: "lcrs-" + v_name
  });
  slider.setSkin('dhx_terrace');
  slider.attachEvent("onChange", function(value) {
    $( "#icrs-" + v_name ).val( $( "#lcrs-" + v_name ).val() );
  });
  $( "#icrs-" + v_name ).on("change", function() {
    var res = parseFloat( $(this).val() );
    if (isNaN(res)) res = 0;
    slider.setValue(res);
    $( this ).val( $("#lcrs-" + v_name ).val());
  });
  $( "#icrs-" + v_name ).val( $( "#lcrs-" + v_name ).val() );
  sliders[v_name] = slider;
  return true;
}

function remove_slider(v_name) {
  if (!(v_name in sliders)) return false;
  sliders[v_name].unload();
  delete sliders[v_name];
  $( "#tcrs-" + v_name ).empty();
  $( '#table-crs' ).remove("#tcrs-" + v_name);
  return true;
}

function remove_sliders() {
  for (name in sliders) {
    sliders[name].unload();
  }
  sliders = {};
  $( '#table-crs' ).empty();
}

function init() {
  callback('init');
  size_changed();
}

$( document ).ready( function() {
  init();
});

$( window ).unload( function() {
  remove_sliders();
});
