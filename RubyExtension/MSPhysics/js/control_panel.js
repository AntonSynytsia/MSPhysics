var g_sliders = {};
var g_interval_timer = null;

function callback(v_name, v_data) {
  if (!v_data) v_data = '';
  window.location.href = 'skp:' + v_name + '@' + v_data;
}

function init() {
  var w = $( window ).width();
  var h = $( window ).height();
  var data = '['+w+','+h+']';
  callback('init', data);
}

function uninit() {
  remove_sliders();
  if (g_interval_timer != null) {
    clearInterval(g_interval_timer);
    g_interval_timer = null;
  }
}

function update_size() {
  var w = $( 'body' ).width();
  var h = $( 'body' ).height();
  var data = '['+w+','+h+']';
  callback('size_changed', data);
}

function add_slider(v_name, v_default_value, v_min, v_max, v_step) {
  if (v_name in g_sliders) return false;
  //var v_name2 = v_name.replace(/\s+/g, '_');
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
    size:   188,
    step:   v_step,
    min:    v_min,
    max:    v_max,
    value:  v_default_value,
    linkTo: "lcrs-" + v_name
  });
  slider.setSkin('dhx_terrace');
  var i_id = "[id=\"icrs-" + v_name + "\"]";
  var l_id = "[id=\"lcrs-" + v_name + "\"]";
  slider.attachEvent("onChange", function(value) {
    //$( i_id ).val( $( l_id ).val() );
    $( i_id ).val( value );
  });
  $( i_id ).on('change', function() {
    var res = parseFloat( $(this).val() );
    if (isNaN(res)) res = 0;
    slider.setValue(res);
    $( this ).val( $( l_id ).val());
  });
  $( i_id ).val( $( l_id ).val() );
  g_sliders[v_name] = slider;
  return true;
}

function update_slider(v_name) {
  if (!(v_name in g_sliders)) return false;
  var i_id = "[id=\"icrs-" + v_name + "\"]";
  var l_id = "[id=\"lcrs-" + v_name + "\"]";
  $( i_id ).val( $( l_id ).val() );
  return true;
}

function remove_slider(v_name) {
  if (!(v_name in g_sliders)) return false;
  g_sliders[v_name].unload();
  delete g_sliders[v_name];
  //var v_name2 = v_name.replace(/\s+/g, '_');
  var t_id = "[id=\"tcrs-" + v_name + "\"]";
  $( t_id ).empty();
  $( '#table-crs' ).remove( t_id );
  return true;
}

function remove_sliders() {
  for (name in g_sliders) {
    g_sliders[name].unload();
  }
  g_sliders = {};
  $( '#table-crs' ).empty();
}

$( document ).ready(function() {
  window.setTimeout(init, 10); // Timeout is needed because without it the event might not be triggered on Mac OS X.
  g_interval_timer = setInterval(function() { callback('update_note') }, 1000);
});

$( window ).unload(function() {
  uninit();
});

$( document ).mouseenter(function() {
  callback('mouse_enter');
});

$( document ).mouseleave(function() {
  callback('mouse_leave');
});
