var g_sliders = {};
var g_interval_timer = null;

function callback(v_name, v_data) {
  if (!v_data) v_data = '';
  window.location.href = 'skp:' + v_name + '@' + v_data;
}

function init() {
  var cw = $(window).width();
  var ch = $(window).height();
  var data = '['+cw+','+ch+']';
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
  var cw = $('#main-wrap').outerWidth(true);
  var ch = $('#main-wrap').outerHeight(true);
  var data = '['+cw+','+ch+']';
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

function process_keys(e) {
  var tag = e.target.tagName.toLowerCase();
  if (tag != 'input' && tag != 'textarea') {
    e.preventDefault();
    return false;
  }
}

function compute_text_size(v_text, v_font, v_size, v_bold, v_italic) {
  $("#test_note-text").css({
    "font-family": v_font.toString(),
    "font-size": v_size.toString() + "px",
    "font-weight": (v_bold ? "bold" : "normal"),
    "font-style": (v_italic ? "italic" : "normal")
  });
  $("#test_note-text").text(v_text);
  var data = "[" + Math.round($("#test_note-text").width() * 1.5) + ", " + Math.round($("#test_note-text").height() * 1.3) + "]";
  callback('test_result', data);
}

$( document ).ready(function() {
  window.setTimeout(init, 10); // Timeout is needed because without it the event might not be triggered on Mac OS X.
  g_interval_timer = setInterval(function() { callback('update_note') }, 1000);

  $( document ).mouseenter(function() {
    callback('mouse_enter');
  });

  $( document ).mouseleave(function() {
    callback('mouse_leave');
  });

  /*window.addEventListener('keydown', function (e) {
    var tag = e.target.tagName.toLowerCase();
    if (tag != 'input' && tag != 'textarea') {
      e.preventDefault();
      return false;
    }
  });*/
  if (window.addEventListener) {
    window.addEventListener('keydown', process_keys);
  } else if (window.attachEvent) {
    window.attachEvent('oneydown', process_keys);
  }

});

$( window ).unload(function() {
  uninit();
});
