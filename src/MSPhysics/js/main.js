String.prototype.contains = function(it) { return this.indexOf(it) != -1; };

function is_int(n) {
   return typeof n === 'number' && parseFloat(n) == parseInt(n, 10) && !isNaN(n);
}

function callback(name, data) {
    if (!data) data = '';
    window.location.href = 'skp:' + name + '@' + data;
}

$(document).on('click', 'a[href]', function() {
    if (this.href.contains("http")){
        callback('open_link', this.href);
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
    var editor = ace.edit('editor');
    editor.getSession().getUndoManager().reset;
    editor.getSession().setValue(code);
}

function editor_set_line(line) {
    var editor = ace.edit('editor');
    editor.gotoLine(line);
}

function editor_select_current_line() {
    var editor = ace.edit('editor');
    editor.selection.selectLineEnd();
}

function activate_tab(number) {
    $('#tab'+number).fadeIn(400).siblings().hide();
    $('a[href$="#tab'+number+'"]').parent('li').addClass('active').siblings().removeClass('active');
}


$(document).ready( function() {

    // Determine browser for compatibility
    var is_chrome = navigator.userAgent.toLowerCase().indexOf('chrome') > -1;

    // Initialize
    window.setTimeout(init, 0);

    // Create some audio
    /*var audio_element = document.createElement('audio');
    var audio_links = [
    ];
    var index = 0;
    $('#ace-url').mouseover(function() {
        audio_element.setAttribute('src', audio_links[index]);
        if (index < audio_links.length-1)
            index += 1;
        else
            index = 0;
        audio_element.setAttribute('autoplay', 'autoplay');
        if (is_chrome)
            audio_element.Play();
        else
            audio_element.play();
    });*/

    // Initialize Ace editor
    var editor = ace.edit("editor");
    var container = document.getElementById("editor");
    var saved = true;
    var StatusBar = ace.require("ace/ext/statusbar").StatusBar;
    var statusbar = new StatusBar(editor, document.getElementById("status-bar"));
    editor.session.setMode("ace/mode/ruby");
    editor.setTheme("ace/theme/dawn");
    editor.setSelectionStyle("text");
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
    editor.setOption("scrollPastEnd", false);
    container.style.fontSize = '12px';
    editor.on("change", function(e) {
        saved = false;
    });

    function editor_changed() {
        if (saved) return;
        var code = editor.getSession().getValue();
        $( '#temp-area' ).val(code);
        callback('editor_changed', '');
        saved = true;
    }

    // Save editor script on the following events.
    $('#editor').focusout(function() {
        editor_changed();
    });
    $('#editor').mouseout(function() {
        //var pos = editor.selection.getSelectionAnchor();
        var pos = editor.getSelection().getCursor();
        if (pos) callback('cursor_changed', pos.row+1);
        editor_changed();
    });
    // Focus editor on mouse enter.
    $('#editor').mouseenter(function() {
        editor.focus();
    });

    // Initialize tabs
    $('.tabs .tab-links a').on('click', function(e) {
        var currentAttrValue = $(this).attr('href');
        // Show/Hide Tabs
        $('.tabs ' + currentAttrValue).fadeIn(400).siblings().hide();
        // Change/remove current tab to active
        $(this).parent('li').addClass('active').siblings().removeClass('active');
        callback('tab_changed', currentAttrValue);
        e.preventDefault();
    });

    // Resize editor
    $( window ).resize( function() {
        container.style.height = $( window ).height() - 119 + "px";
        editor.resize();
    });
    $( window ).resize();

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
    $('.numeric-input').focusout(function() {
        try {
            with (Math) {
                var num = eval(this.value);
            }
            if (typeof num != 'number')
                throw 0;
        }
        catch(err) {
            var num = 0;
        }
        this.value = num.toFixed(2);
    });

    // Randomize body background
    /*if (Math.floor(Math.random() * 2) == 1){
        $( 'body' ).css("background", "rgb(180,130,190)");
        $( 'body' ).css("background", "-moz-linear-gradient(top,  rgba(180,130,190,1) 0%, rgba(182,150,188,1) 100%)");
        $( 'body' ).css("background", "-webkit-gradient(linear, left top, left bottom, color-stop(0%,rgba(180,130,190,1)), color-stop(100%,rgba(182,150,188,1)))");
        $( 'body' ).css("background", "-webkit-linear-gradient(top,  rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
        $( 'body' ).css("background", "-o-linear-gradient(top,   rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
        $( 'body' ).css("background", "-ms-linear-gradient(top,  rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
        $( 'body' ).css("background", "linear-gradient(to bottom,   rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
        $( 'body' ).css("filter", "progid:DXImageTransform.Microsoft.gradient( startColorstr='#b482be', endColorstr='#b696bc',GradientType=0 )");
    }*/
});
