var g_last_cursor = [1,0];
var g_active_tab_id = 1;
var g_size_update_time = null;
var g_editor_size_offset = 2;
var g_script_saved = true;
var g_normal_width = 470;
var g_kns_velocity = null;
var g_kns_omega = null;
var g_kns_atime = null;

var g_use_html_dialog = false;
var g_ms_ie = null;
var g_chrome = null;

String.prototype.contains = function(it) { return this.indexOf(it) != -1; };

function is_int(n) {
    return typeof n === 'number' && parseFloat(n) == parseInt(n, 10) && !isNaN(n);
}

function type(obj) {
    return Object.prototype.toString().call(obj).slice(8, -1);
}

function eval_string_to_float(str) {
    var num = 0.0;
    try {
        with (Math) {
            num = eval(str);
        }
    }
    catch(err) {
        return 0.0;
    }
    if (typeof num != "number" || num === NaN || num === Infinity)
        return 0.0;
    return num;
}

function clamp_number(number, min, max) {
    if (number < min) {
        return min;
    }
    else if (number > max) {
        return max;
    }
    else {
        return number;
    }
}

function callback(name, data) {
    if (!data)
        data = "";
    if (typeof sketchup === "undefined") {
        window.location.href = "skp:" + name + "@" + data;
    }
    else {
        var f = window["sketchup"][name];
        if (f !== undefined) f(data);
    }
}

function init() {
    var cw = $(window).width();
    var ch = $(window).height();
    var data = '[' + cw + ',' + ch + ']';
    callback('init', data);
}

function editor_set_script(code) {
    //window.aceEditor.focus();
    $('#temp_script_area').val(code);
    window.aceEditor.getSession().getUndoManager().reset;
    window.aceEditor.getSession().setValue(code);
}

function editor_set_cursor(row, col) {
    window.aceEditor.gotoLine(row, col, true);
    g_last_cursor = [row, col];
}

function editor_select_current_line() {
    window.aceEditor.selection.selectLineEnd();
}

function update_editor_placement() {
    if (g_active_tab_id == 3) {
        var value = $(window).height() - $('#main-wrap').outerHeight(true) + $('#editor').innerHeight() + g_editor_size_offset;
        if (value < 0) value = 0;
        document.getElementById('editor').style.height = value + 'px';
        window.aceEditor.resize();
    }
}

function update_editor_size() {
    if (g_active_tab_id == 3) {
        var cw = $(window).width();
        var ch = $(window).height();
        var data = '['+cw+','+ch+']';
        callback('editor_size_changed', data);
    }
}

function update_placement(tab_id, bresize) {
    g_size_update_time = new Date();
    bresize = bresize ? true : false;
    if (tab_id != 0) {
        $('#tab' + tab_id).fadeIn(150).siblings().hide();
        $('a[href$="#tab' + tab_id + '"]').parent('li').addClass('active').siblings().removeClass('active');
        g_active_tab_id = tab_id;
    }
    $('.help_box').val("");
    var dw = g_normal_width;
    var dh = $('#main-wrap').outerHeight(true);
    var data = '[' + g_active_tab_id + ',' + dw + ',' + dh + ',' + bresize + ']';
    callback('placement_changed', data);
    update_editor_placement();
}

function assign_joint_click_event() {
    var last_selected_joint_label = 0;
    $('.joint-label').off();
    $('.joint-label').on('click', function(evt) {
        evt.preventDefault();
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
    $('.slider-input').off();
    $('.text-input').off();
    $('input[type=checkbox], input[type=radio]').off();
    $('.button').off();

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
        var num = eval_string_to_float(this.value);
        var data = '["' + this.id + '",' + num + ']';
        callback('numeric_input_changed', data);
    });

    $('.fixnum-input').on('focusout', function() {
        var num = eval_string_to_float(this.value);
        var data = '["' + this.id + '",' + num + ']';
        callback('fixnum_input_changed', data);
    });

    $('.controller-input').on('focusout', function() {
        if (g_use_html_dialog) {
            var data = [this.id, this.value];
            callback('controller_input_changed', data);
        }
        else {
            callback('controller_input_changed', this.id);
        }
    });

    $('.slider-input').on('focusout', function() {
        var num = eval_string_to_float(this.value);
        var data = '["' + this.id + '",' + num + ']';
        callback('slider_input_changed', data);
    });

    $('.text-input').on('focusout', function() {
        if (g_use_html_dialog) {
            var data = [this.id, this.value];
            callback('text_input_changed', data);
        }
        else {
            callback('text_input_changed', this.id);
        }
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
            update_placement(0, true);
        }
        var data = '["' + this.id + '",' + this.checked + ']';
        callback('check_input_changed', data);
    });

    // Process button clicks
    $('.button').on('click', function(evt) {
        callback('button_clicked', this.id);
        evt.preventDefault();
    });
}

function display_help_box(state) {
    $('.help_box').css('display', state ? 'inline-block': 'none');
}

function editor_changed() {
    if (g_script_saved) return;
    var code = window.aceEditor.getSession().getValue();
    // Return if no changes made
    if ($('#temp_script_area').val() == code) return;
    $('#temp_script_area').val(code);
    callback('editor_changed', code);
    g_script_saved = true;
}

function set_viewport_scale(scale) {
    var percent = Math.round(100.0 / parseFloat(scale));
    $("style[id='vp_scale']").remove();
    $("<style id=\"vp_scale\" type='text/css'>@-ms-viewport { width: " + percent + "%; }</style>").appendTo("head");
}

function create_slider(v_id, v_size, v_step, v_min, v_max, v_value) {
    var slider = new dhtmlXSlider({
        parent: v_id + "-slider",
        size: v_size,
        step: v_step,
        min: v_min,
        max: v_max,
        value: v_value,
        linkTo: v_id
    });
    slider.setSkin('dhx_terrace');
    slider.attachEvent("onChange", function(value) {
        var data = '["' + v_id + '",' + value + ']';
        callback('slider_changed', data);
    });
    return slider;
}

$(document).on('click', 'a[href]', function(evt) {
    evt.preventDefault();
    if (this.href.contains('http://') || this.href.contains('https://')) {
        callback('open_link', this.href);
        return false;
    }
});

$(document).ready( function() {
    g_size_update_time = new Date();

    if (typeof sketchup === "undefined") {
        g_use_html_dialog = false;
    }
    else {
        g_use_html_dialog = true;
    }

    // Handle errors
    window.onerror = function(msg, url, linenumber) {
        alert("Error: " + msg + "\nLine: " + linenumber);
        return true;
    };

    // Initialize Chosen
    var config = {
        '.chosen-select'           : {},
        '.chosen-select-deselect'  : {allow_single_deselect:true},
        '.chosen-select-no-single' : {disable_search_threshold:6},
        '.chosen-select-no-results': {no_results_text:'Oops, nothing found!'},
        '.chosen-select-width'     : {width:"100%"},
        '.chosen-select-custom'    : {width:"100%", disable_search_threshold:6, no_results_text:'No matches!'}
    }
    for (var selector in config) {
        $(selector).chosen(config[selector]);
    }

    // Initialize expander
    $('.expander').simpleexpand({'hideMode' : 'slideToggle'});

    $('.expander').on('click', function(evt) {
        evt.preventDefault();
        for (i = 1; i < 8; i++) {
            window.setTimeout(function() { update_placement(0, true); }, i * 25);
        }
    });

    // Initialize keyboard navigation sliders
    if (g_kns_velocity == null) g_kns_velocity = create_slider("simulation-key_nav_velocity", 126, 1, 1, 200, 50);
    if (g_kns_omega == null) g_kns_omega = create_slider("simulation-key_nav_omega", 126, 0.1, 0.1, 10, 2);
    if (g_kns_atime == null) g_kns_atime = create_slider("simulation-key_nav_atime", 126, 0.01, 0, 5, 0.5);

    // Initialize Ace
    var dom = ace.require("ace/lib/dom");
    ace.require("ace/ext/language_tools");
    var StatusBar = ace.require('ace/ext/statusbar').StatusBar;
    var editor = ace.edit('editor');
    window.aceEditor = editor;
    var container = document.getElementById('editor');
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
    editor.setOptions({
        scrollPastEnd: true,
        enableBasicAutocompletion: true
    });
    container.style.fontSize = '12px';
    editor.renderer.setScrollMargin(10, 10);
    container.style.fontFamily = 'Courier New';
    editor.getSession().on('change', function(e) {
        g_script_saved = false;
    });

    editor.commands.addCommand({
        name: "showKeyboardShortcuts",
        bindKey: {win: "Ctrl-Alt-h", mac: "Command-Alt-h"},
        exec: function(ed) {
            ace.config.loadModule("ace/ext/keybinding_menu", function(module) {
                module.init(ed);
                ed.showKeyboardShortcuts();
            });
        }
    });
    //editor.execCommand("showKeyboardShortcuts");

    editor.commands.addCommand({
        name: "Toggle Fullscreen",
        bindKey: "F11",
        exec: function(ed) {
            var fs = dom.toggleCssClass(document.body, "fullscreen");
            dom.setCssClass(ed.container, "fullscreen", fs);
            ed.setAutoScrollEditorIntoView(!fs);
            update_editor_placement();
        }
    });

    // Save editor script on the following events.
    $('#editor').focusout(function() {
        editor_changed();
    });

    $('#editor').mouseout(function() {
        var pos = editor.selection.getSelectionAnchor();
        //var pos = editor.getSelection().getCursor();
        if (pos && (pos.row+1 != g_last_cursor[0] || pos.column != g_last_cursor[1])) {
            g_last_cursor = [pos.row+1, pos.column];
            var data = '[' + g_last_cursor[0] + ',' + g_last_cursor[1] + ']';
            callback('cursor_changed', data);
        }
        editor_changed();
    });

    // Focus editor on mouse enter.
    /*$('#editor').mouseenter(function() {
        editor.focus();
    });*/

    // Initialize tabs
    $('.tabs .tab-links a').on('click', function(evt) {
        evt.preventDefault();
        var currentAttrValue = $(this).attr('href');
        // Change/remove current tab to active
        $(this).parent('li').addClass('active').siblings().removeClass('active');
        var tab_id = currentAttrValue.charAt(currentAttrValue.length - 1);
        update_placement(tab_id, true);
    });

    $(document).on('mouseenter', function() {
        var data = '';
        callback('mouse_enter', data);
    });

    // Resize editor
    $(window).resize( function(evt) {
        evt.preventDefault();
        var current_time = new Date();
        if (g_active_tab_id == 3 && current_time - g_size_update_time > 300) {
            update_editor_placement();
            update_editor_size();
        }
    });

    $('.drop-down').on('change', function(evt, params) {
        evt.preventDefault();
        if (this.id == 'editor-theme') {
            editor.setTheme('ace/theme/' + params.selected);
        }
        else if (this.id == 'editor-font') {
            container.style.fontSize = params.selected + 'px';
        }
        else if (this.id == 'editor-wrap') {
            editor.setOption('wrap', params.selected);
        }
        else if (this.id == 'body-mass_control') {
            if (params.selected == '2') {
                $('#body-control_by_density').css('display', 'none');
                $('#body-control_by_mass').css('display', 'table-row');
            }
            else {
                $('#body-control_by_density').css('display', 'table-row');
                $('#body-control_by_mass').css('display', 'none');
            }
            window.setTimeout( function() { update_placement(0, true); }, 50);
        }
        else if (this.id == 'body-magnet_mode') {
            if (params.selected == '2') {
                $('#body-magnet_mode1a').css('display', 'none');
                $('#body-magnet_mode1b').css('display', 'none');
                $('#body-magnet_mode2').css('display', 'table-row');
            }
            else {
                $('#body-magnet_mode1a').css('display', 'table-row');
                $('#body-magnet_mode1b').css('display', 'table-row');
                $('#body-magnet_mode2').css('display', 'none');
            }
            window.setTimeout( function() { update_placement(0, true); }, 50);
            }
        else if (this.id == 'dialog-scale') {
            set_viewport_scale(params.selected);
            window.setTimeout( function() { update_placement(1, true); }, 50);
            window.setTimeout( function() { update_placement(1, true); }, 150);
        }
        else if (this.id == 'hinge-mode') {
            if (params.selected == '2') {
                $('.hinge-mode_s0').css('display', 'none');
                $('.hinge-mode_s1').css('display', 'none');
                $('.hinge-mode_s2').css('display', 'table-row');
                $('.hinge-mode_s12').css('display', 'table-row');
                $('#hinge-start_angle').css('display', 'table-row');
            }
            else if (params.selected == '1') {
                $('.hinge-mode_s0').css('display', 'none');
                $('.hinge-mode_s1').css('display', 'table-row');
                $('.hinge-mode_s2').css('display', 'none');
                $('.hinge-mode_s12').css('display', 'table-row');
                $('#hinge-start_angle').css('display', 'table-row');
            }
            else {
                $('.hinge-mode_s0').css('display', 'table-row');
                $('.hinge-mode_s1').css('display', 'none');
                $('.hinge-mode_s2').css('display', 'none');
                $('.hinge-mode_s12').css('display', 'none');
                $('#hinge-start_angle').css('display', 'none');
            }
            window.setTimeout( function() { update_placement(0, true); }, 50);
        }
        else if (this.id == 'spring-mode') {
            if (params.selected == '1') {
                $('.spring-mode_s0').css('display', 'none');
                $('.spring-mode_s1').css('display', 'table-row');
            }
            else {
                $('.spring-mode_s0').css('display', 'table-row');
                $('.spring-mode_s1').css('display', 'none');
            }
            window.setTimeout( function() { update_placement(0, true); }, 50);
        }
        var data = '["' + this.id + '","' + params.selected + '"]';
        callback('select_input_changed', data);
    });

    $('#sound-list').on('change', function() {
        callback('sound_select_changed', this.value);
    });

    update_input_events();

    $('a,p,label,input,button').mouseover(function() {
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
            $('.help_box').val("");
        }
    });

    // Initialize
    window.setTimeout(init, 0);

});
