ace.define("ace/theme/msphysics_light",["require","exports","module","ace/lib/dom"], function(e, t, n) {

t.isDark = false;
t.cssClass = "ace-msphysics_light";
t.cssText = "\
.ace-msphysics_light .ace_gutter {\
background-color: #DDD;\
color: #555;\
border-right: 1px solid #BBB;\
}\
.ace-msphysics_light { 								/* GLOBAL bg and color */\
background-color: #EEE;\
color: #333;\
}\
.ace-msphysics_light .ace_variable { 			/* 	@ sign   */\
color:#86C;\
}\
.ace-msphysics_light .ace_paren { 				/*     () {}     */\
color: #333;\
}\
.ace-msphysics_light .ace_cursor {\
color: #000;\
}\
.ace-msphysics_light .ace_constant { 			/* true, false, nil, ... */\
color:#6B8;\
}\
.ace-msphysics_light .ace_numeric {\
color:#6A8;\
}\
.ace-msphysics_light .ace_keyword { 			/* if, elsif, else, ... + body events*/\
color:#F64;\
}\
.ace-msphysics_light .ace_mp_keywords { 	/* MSPhysics methods*/\
color:#EA5;\
}\
.ace-msphysics_light .ace_support {\
color:#A86;\
}\
.ace-msphysics_light .ace_operator {\
color: #000;\
}\
.ace-msphysics_light .ace_function {\
color:#F33;\
}\
.ace-msphysics_light .ace_string {\
color:#AB4;\
}\
.ace-msphysics_light .ace_comment {\
color: #AAA;\
}\
.ace-msphysics_light .ace_marker-layer .ace_selection {\
background-color: #BCD;\
border-radius: 0;\
}\
.ace-msphysics_light .ace_marker-layer .ace_selected-word {\
border: 1px solid #AFA;\
box-sizing: border-box;\
background-color: #ADA;\
}\
.ace-msphysics_light .ace_bracket {\
margin: -1px 0 0 -1px;\
border: 1px solid #888;\
}\
.ace-msphysics_light .ace_marker-layer .ace_step {\
background-color: rgb(198, 219, 174);\
}\
.ace-msphysics_light .ace_marker-layer .ace_active-line {\
background-color: rgba(100, 100, 100, 0.15);\
}\
.ace-msphysics_light .ace_gutter-active-line {\
background-color: rgba(100, 100, 100, 0.2);\
}\
.ace-msphysics_light .ace_invisible {\
color: #BFBFBF\
}\
.ace-msphysics_light .ace_storage {\
color:#A52A2A;\
}\
.ace-msphysics_light .ace_invalid.ace_illegal {\
color:#FD1224;\
background-color:rgba(255, 6, 0, 0.15);\
}\
.ace-msphysics_light .ace_invalid.ace_deprecated {\
text-decoration:underline;\
font-style:italic;\
color:#FD1732;\
background-color:#E8E9E8;\
}\
.ace-msphysics_light .ace_regexp{\
color:#FFF;\
background-color: rgba(100,50,50,0.5);\
}\
.ace-msphysics_light .ace_meta.ace_tag {\
color:#005273;\
}\
.ace-msphysics_light .ace_markup.ace_heading {\
color:#B8012D;\
background-color:rgba(191, 97, 51, 0.051);\
}\
.ace-msphysics_light .ace_markup.ace_list{\
color:#8F5B26;\
}\
.ace-msphysics_light .ace_print-margin {\
width: 1px;\
background-color: rgba(0,0,0,0);\
}";

var r = e("../lib/dom");
r.importCssString(t.cssText, t.cssClass);
});
