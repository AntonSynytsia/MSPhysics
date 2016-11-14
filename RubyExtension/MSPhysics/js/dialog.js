var last_cursor = [1,0];
var active_tab_id = 1;
var size_updating = false;
var editor_size_offset = 3;

String.prototype.contains = function(it) { return this.indexOf(it) != -1; };

function is_int(n) {
  return typeof n === 'number' && parseFloat(n) == parseInt(n, 10) && !isNaN(n);
}

function type(obj) {
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
  //window.aceEditor.focus();
  $( '#temp_script_area' ).val(code);
  window.aceEditor.getSession().getUndoManager().reset;
  window.aceEditor.getSession().setValue(code);
}

function editor_set_cursor(row, col) {
  window.aceEditor.gotoLine(row, col, true);
  last_cursor = [row, col];
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
  size_updating = true;
  //var id = '#tab' + active_tab_id;
  var w = 462; // 11px * 48em
  var h = $('#body').outerHeight(true);
  var value = $(window).height() - h + $('#editor').innerHeight() + editor_size_offset;
  if (value < 0) value = 0;
  document.getElementById('editor').style.height = value + 'px';
  var data = '['+w+','+h+']';
  callback('size_changed', data);
  // Repeat because it doesn't always work.
  var h = $('#body').outerHeight(true);
  var value = $(window).height() - h + $('#editor').innerHeight() + editor_size_offset;
  if (value < 0) value = 0;
  document.getElementById('editor').style.height = value + 'px';
  window.aceEditor.resize();
  var data = '['+w+','+h+']';
  callback('size_changed', data);
  size_updating = false;
}

function update_size2() {
  size_updating = true;
  //var id = '#tab' + active_tab_id;
  var w = 462; // 11px * 48em
  var h = $('#body').outerHeight(true);
  var value = $(window).height() - h + $('#editor').innerHeight() + editor_size_offset;
  if (value < 0) value = 0;
  document.getElementById('editor').style.height = value + 'px';
  var data = '['+w+','+h+']';
  callback('size_changed', data);
  size_updating = false;
}

function update_editor_size() {
  var w = $( window ).width();
  var h = $( window ).height();
  var data = '['+w+','+h+']';
  callback('editor_size_changed', data);
}

function assign_joint_click_event() {
  var last_selected_joint_label = 0;
  $('.joint-label').off();
  $('.joint-label').on('click', function() {
    if (last_selected_joint_label != 0) {
      last_selected_joint_label.removeClass('joint-label-selected');
      last_selected_joint_label.addClass('joint-label');
    }
    $(this).removeClass('joint-label');
    $(this).addClass('joint-label-selected');
    last_selected_joint_label = $(this);
    callback('joint_label_selected', this.id);
  });
}

function update_input_events() {
  $('.numeric-input').off();
  $('.fixnum-input').off();
  $('.controller-input').off();
  $('.text-input').off();
  $('input[type=checkbox], input[type=radio]').off();
  $('button').off();

  // Validate numeric input.
  /*$('.numeric-input').on('keypress', function(evt) {
    var key = evt.keyCode || evt.which;
    key = String.fromCharCode( key );
    var regex = /[0-9]|\.|\-/;
    if( !regex.test(key) ) {
      evt.returnValue = false;
      if(evt.preventDefault) evt.preventDefault();
    }
  });*/

  // $('.numeric-input').attr("maxlength", 64);

  $('.numeric-input').on('focusout', function() {
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

  $('.fixnum-input').on('focusout', function() {
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

  $('.controller-input').on('focusout', function() {
    callback('controller_input_changed', this.id);
  });

  $('.text-input').on('focusout', function() {
    callback('text_input_changed', this.id);
  });

  $('.numeric-input').on('focusin', function() {
    callback('numeric_input_focused', this.id);
  });

  // Process checkbox and radio input triggers.
  $('input[type=checkbox], input[type=radio]').on('change', function() {
    if (this.id == 'editor-read_only') {
      window.aceEditor.setReadOnly(this.checked);
    }
    else if (this.id == 'editor-print_margin') {
      window.aceEditor.setShowPrintMargin(this.checked);
    }
    else if (this.id == 'dialog-help_box') {
      display_help_box(this.checked);
    }
    var data = '["'+this.id+'",'+this.checked+']';
    callback('check_input_changed', data);
  });

  // Process button clicks
  $('button').on('click', function() {
    callback('button_clicked', this.id);
  });
}

function display_help_box(state) {
  $('.help_box').css('display', state ? 'inline-block': 'none');
  update_size2();
}

$(document).ready( function() {

  // Determine browser for compatibility
  var is_chrome = navigator.userAgent.toLowerCase().indexOf('chrome') > -1;

  // Initialize
  window.setTimeout(init, 10); // Timeout is needed because without it the event might not be triggered on Mac OS X.

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

  // Initialize expander
  $('.expander').simpleexpand({'hideMode' : 'slideToggle'});

  $('.expander').click(function() {
    for (i = 1; i < 8; i++) {
      window.setTimeout(function() { update_size2() }, i * 25);
    }
  });

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
  editor.session.setUseWrapMode(true);
  editor.setHighlightSelectedWord(true);
  editor.setBehavioursEnabled(true);
  editor.setFadeFoldWidgets(true);
  editor.setOption('scrollPastEnd', true);
  container.style.fontSize = '12px';
  container.style.fontFamily = 'Courier New';
  editor.getSession().on('change', function(e) {
    script_saved = false;
  });

  editor.commands.addCommand({
    name: "showKeyboardShortcuts",
    bindKey: {win: "Ctrl-Alt-h", mac: "Command-Alt-h"},
    exec: function(editor) {
      ace.config.loadModule("ace/ext/keybinding_menu", function(module) {
        module.init(editor);
        editor.showKeyboardShortcuts()
      })
    }
  })
  //editor.execCommand("showKeyboardShortcuts")

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
    // Change/remove current tab to active
    $(this).parent('li').addClass('active').siblings().removeClass('active');
    var tab_id = currentAttrValue.charAt(currentAttrValue.length-1);
    if( tab_id != active_tab_id ) {
      active_tab_id = tab_id;
      window.setTimeout(function() {
        if (tab_id != 3) update_editor_size();
        callback('tab_changed', tab_id);
      }, 0);
    }
    window.setTimeout(function() { update_size(); }, 0);
    e.preventDefault();
    // Show/Hide Tabs
    $('.tabs ' + currentAttrValue).fadeIn(400).siblings().hide();
    $('.help_box').val("");
  });

  $( document ).on('mouseenter', function(e) {
    callback("mouse_enter");
  });

  // Resize editor
  $( window ).resize( function() {
    if (active_tab_id == 3 && size_updating == false) {
      value = $(window).height() - $('#body').outerHeight(true) + $('#editor').innerHeight() + editor_size_offset;
      if (value < 0) value = 0;
      container.style.height = value + 'px';
      editor.resize();
      update_editor_size();
    }
  });

  $('.drop-down').on('change', function(evt, params) {
    if (this.id == 'editor-theme') {
      editor.setTheme('ace/theme/' + params.selected);
    }
    else if (this.id == 'editor-font') {
      container.style.fontSize = params.selected + 'px';
    }
    else if (this.id == 'editor-wrap') {
      editor.setOption('wrap', params.selected);
    }
    else if (this.id == 'body-weight_control') {
      if (params.selected == 'Mass') {
        $('#body-density_control').css('display', 'none');
        $('#body-mass_control').css('display', 'table-row');
      }
      else {
        $('#body-density_control').css('display', 'table-row');
        $('#body-mass_control').css('display', 'none');
      }
    }
    var data = '["'+this.id+'","'+params.selected+'"]';
    callback('select_input_changed', data);
  });

  $('#sound-list').on('change', function() {
    callback('sound_select_changed', this.value);
  });

  update_input_events();

  $('label,input,button').mouseover(function() {
    try {
      var p1 = this.parentElement;
      var p2 = p1 ? p1.parentElement : null;

      if (this.title != "") {
        $('.help_box').val(this.title);
      }
      else if (p1 && p1.title != "") {
        $('.help_box').val(p1.title);
      }
      else if (p2 && p2.title != "") {
        $('.help_box').val(p2.title);
      }
      else {
        $('.help_box').val("");
      }
    } catch(err) {
      //~ alert(err);
      $('.help_box').val("");
    }
  });

});
