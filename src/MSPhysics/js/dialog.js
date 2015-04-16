var last_cursor = [1,0];
var active_tab_id = 1;

String.prototype.contains = function(it) { return this.indexOf(it) != -1; };

function is_int(n) {
  return typeof n === 'number' && parseFloat(n) == parseInt(n, 10) && !isNaN(n);
}

function type(obj){
  return Object.prototype.toString.call(obj).slice(8, -1);
}

function callback(name, data) {
  if (!data) data = '';
  window.location.href = 'skp:' + name + '@' + data;
}

$(document).on('click', 'a[href]', function() {
  if (this.href.contains("http")) {
    callback('open_link', this.href);
    return false;
  } else if (this.href.contains('ruby_core')) {
    callback('open_ruby_core', '');
    return false;
  }
});

function init() {
  var w = $( window ).width();
  var h = $( window ).height();
  var data = '['+w+','+h+']';
  callback('init', data);
}

function editor_set_script(code) {
  // window.aceEditor.focus();
  $( '#temp_script_area' ).val(code);
  window.aceEditor.getSession().getUndoManager().reset;
  window.aceEditor.getSession().setValue(code);
}

function editor_set_cursor(row, col) {
  var timer = setInterval(function() {
    window.aceEditor.gotoLine(row, col, true);
    last_cursor = [row, col];
    window.clearInterval(timer);
  }, 50);
}

function editor_select_current_line() {
  window.aceEditor.selection.selectLineEnd();
}

function activate_tab(number) {
  $('#tab'+number).fadeIn(400).siblings().hide();
  $('a[href$="#tab'+number+'"]').parent('li').addClass('active').siblings().removeClass('active');
  active_tab_id = number;
  callback('tab_changed', number);
}

function update_size() {
  //var id = '#tab' + active_tab_id;
  var w = 576;
  var h = $( '.tabs' ).height() + 18;
  var data = '['+w+','+h+']';
  callback('update_size', data);
  document.getElementById('editor').style.height = $( window ).height() - 118 + "px";
  window.aceEditor.resize();
}

$(document).ready( function() {

  // Determine browser for compatibility
  var is_chrome = navigator.userAgent.toLowerCase().indexOf('chrome') > -1;

  // Initialize
  window.setTimeout(init, 0);

  // Initialize Chosen
  var config = {
    '.chosen-select'           : {},
    '.chosen-select-deselect'  : {allow_single_deselect:true},
    '.chosen-select-no-single' : {disable_search_threshold:6},
    '.chosen-select-no-results': {no_results_text:'Oops, nothing found!'},
    '.chosen-select-width'     : {width:"95%"}
  }
  for (var selector in config) {
    $( selector ).chosen(config[selector]);
  }

  // Initialize Ace
  var editor = ace.edit('editor');
  window.aceEditor = editor;
  var script_saved = true;
  var container = document.getElementById('editor');
  var StatusBar = ace.require('ace/ext/statusbar').StatusBar;
  var statusbar = new StatusBar(editor, document.getElementById('status-bar'));
  editor.session.setMode('ace/mode/ruby');
  editor.setTheme('ace/theme/tomorrow_night');
  editor.setSelectionStyle('text');
  editor.setHighlightActiveLine(true);
  editor.setShowInvisibles(false);
  editor.setDisplayIndentGuides(true);
  editor.setAnimatedScroll(true);
  editor.renderer.setShowGutter(true);
  editor.renderer.setShowPrintMargin(true);
  editor.session.setUseSoftTabs(true);
  editor.session.setTabSize(2);
  editor.session.setUseWrapMode(false);
  editor.setHighlightSelectedWord(true);
  editor.setBehavioursEnabled(true);
  editor.setFadeFoldWidgets(true);
  editor.setOption('scrollPastEnd', false);
  container.style.fontSize = '12px';
  editor.getSession().on('change', function(e) {
    script_saved = false;
  });

  function editor_changed() {
    if (script_saved) return;
    var code = editor.getSession().getValue();
    // Return if no changes made
    if ($( '#temp_script_area' ).val() == code) return;
    $( '#temp_script_area' ).val(code);
    callback('editor_changed', '');
    script_saved = true;
  }

  // Save editor script on the following events.
  $('#editor').focusout(function() {
    editor_changed();
  });

  $('#editor').mouseout(function() {
    var pos = editor.selection.getSelectionAnchor();
    //var pos = editor.getSelection().getCursor();
    if (pos && (pos.row+1 != last_cursor[0] || pos.column != last_cursor[1])) {
      last_cursor = [pos.row+1, pos.column];
      callback('cursor_changed', '['+last_cursor+']');
    }
    editor_changed();
  });

  // Focus editor on mouse enter.
  /*$('#editor').mouseenter(function() {
    editor.focus();
  });*/

  // Initialize tabs
  $('.tabs .tab-links a').on('click', function(e) {
    var currentAttrValue = $(this).attr('href');
    // Show/Hide Tabs
    $('.tabs ' + currentAttrValue).fadeIn(400).siblings().hide();
    // Change/remove current tab to active
    $(this).parent('li').addClass('active').siblings().removeClass('active');
    var tab_id = currentAttrValue.charAt(currentAttrValue.length-1);
    if( tab_id != active_tab_id ){
      active_tab_id = tab_id;
      callback('tab_changed', tab_id);
    }
    update_size();
    e.preventDefault();
  });

  $('#tab1').on('mouseenter', function(e) {
    callback('update_simulation_state');
  });

  // Resize editor
  $( window ).resize( function() {
    if( active_tab_id == 3 ){
      container.style.height = $( window ).height() - 118 + "px";
      editor.resize();
    }
  });

  // Validate numeric input.
  /*$('.numeric-input').keypress(function(evt) {
    var key = evt.keyCode || evt.which;
    key = String.fromCharCode( key );
    var regex = /[0-9]|\.|\-/;
    if( !regex.test(key) ) {
      evt.returnValue = false;
      if(evt.preventDefault) evt.preventDefault();
    }
  });*/

  // $('.numeric-input').attr("maxlength", 64);

  $('.numeric-input').focusout( function() {
    try {
      with (Math) {
        var num = eval(this.value);
      }
      if (typeof num != 'number' || num === NaN || num === Infinity) throw 0;
    }
    catch(err) {
      var num = 0.0;
    }
    var data = '["'+this.id+'",'+num+']';
    callback('numeric_input_changed', data);
  });

  $('.fixnum-input').focusout( function() {
    try {
      with (Math) {
        var num = eval(this.value);
      }
      if (typeof num != 'number' || num === NaN || num === Infinity) throw 0;
    }
    catch(err) {
      var num = 0.0;
    }
    var data = '["'+this.id+'",'+num+']';
    callback('fixnum_input_changed', data);
  });

  $('.controller-input').focusout( function() {
    callback('controller_input_changed', this.id);
  });

  $('.numeric-input').focusin( function() {
    callback('numeric_input_focused', this.id);
  });

  // Process checkbox and radio input triggers.
  $('input[type=checkbox], input[type=radio]').change( function() {
    var data = '["'+this.id+'",'+this.checked+']';
    callback('check_input_changed', data);
  });

  $('.drop-down').on('change', function(evt, params) {
    var data = '["'+this.id+'","'+params.selected+'"]';
    callback('select_input_changed', data);
  });

  $('#sound-list').on('change', function() {
    callback('sound_select_changed', this.value);
  });

  // Process button clicks
  $('button').click( function() {
    callback('button_clicked', this.id);
  });

});
