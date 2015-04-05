// -------------------------------+
// CARPE Slider 2.0.8             |
// CARPE Common 0.7               |
// 2012-09-13                     |
// By Tom Hermansson Snickars     |
// Copyright CARPE Design         |
// http://carpe.ambiprospect.com/ |
//--------------------------------+
(function () {
    'use strict';
    var CARPE,
        doc = window.document;
    window.CARPE = CARPE = {
        version: '0.7',
        domLoaded: function (callback) {
            if (doc.addEventListener) { // Mozilla, Chrome, Opera:
                doc.addEventListener('DOMContentLoaded', callback, false);
            } else if (/KHTML|WebKit|iCab/i.test(navigator.userAgent)) {
            // Safari, iCab, Chrome, Firefox, Konqueror:
                var DOMLoadTimer = setInterval(function () {
                    if (/loaded|complete/i.test(doc.readyState)) {
                        callback();
                        clearInterval(DOMLoadTimer);
                    }
                }, 10);
            } else {
                window.onload = callback; // Other web browsers:
            }
        },
        isFunction: function (func) {
            return !!(func && func.constructor && func.call && func.apply);
        },
        // transfer: transfers properties of source to a target object.
        transfer: function (source, target, targetPriority) {
            var i;
            for (i in source) {
                if (Object.prototype.hasOwnProperty.call(source, i) &&
                        (!targetPriority || (target[i] === undefined))) {
                    target[i] = source[i];
                }
            }
        },
        // bind: Binds a function to an object with the correct
        // reference for the 'this' keyword.
        bind: function (obj) {
            var method = this,
                temp = function () {
                    return method.apply(obj, arguments);
                };
            return temp;
        },
        // roundToDec: Returns a number rounded off to a specified
        // number of decimals.
        roundToDec: function (num, decimals, fill) {
            var dec,
                i,
                len,
                temp = Math.pow(10, decimals),
                result = (Math.round(num * temp) / temp).toString(),
                point = result.indexOf('.');
            result = (point < 0) ? result : result.substr(0,
                    (point + decimals + 1));
            if (fill) {
                if ((point < 0) && (decimals > 0)) {
                    result += '.';
                    point = result.length - 1;
                }
                len = result.length;
                if ((len > point) && (point > -1)) {
                    dec = result.substring((point + 1), (len));
                    len = dec.length;
                } else {
                    len = 0;
                }
                if (len < decimals) {
                    for (i = len; i < decimals; i += 1) {
                        result = result + '0';
                    }
                }
            }
            return result;
        },
        // getElementsByClass: Returns an array with all elements that
        // have a class attribute that contains 'className'.
        getElementsByClass: function (className) {
            var classElements = [],
                els = doc.getElementsByTagName("*"),
                i,
                j = 0,
                len = els.length || 0,
                pattern = new RegExp("(^|\\s)" + className + "(\\s|$)");
            for (i = 0; i < len; i += 1) {
                if (pattern.test(els[i].className)) {
                    classElements[j] = els[i];
                    j += 1;
                }
            }
            return classElements;
        },
        // position: handles both horizontal and vertical relative positioning
        // of elements. Returns or sets the position of an element as an
        // object: { 'x': x, 'y': y }. Bind as method to element!
        position: function (pos) {
            var elmnt = this,
                result = this;
            function modPos(dir, val) {
                var style = elmnt.style,
                    unit = 'px';
                val = (val || (val === 0)) ? val : null;
                if (style) {
                    if (typeof (style[dir]) === 'string') {
                        if (val !== null) { // setter.
                            style[dir] = val.toString() + unit;
                        } else { // getter.
                            val = parseInt(style[dir], 10);
                        }
                    } else if (style[CARPE.camelize('pixel' + dir)]) {
                        if (val !== null) { // setter.
                            style[CARPE.camelize('pixel' + dir)] = val;
                        } else { // getter.
                            val = style[CARPE.camelize('pixel' + dir)];
                        }
                    }
                }
            }
            if (pos) { // setter.
                if (pos.x || pos.x === 0) {
                    modPos('left', pos.x);
                }
                if (pos.y || pos.y === 0) {
                    modPos('top', pos.y);
                }
            } else { // getter.
                result = { x: modPos('left'), y: modPos('top') };
            }
            return result;
        },
        getPos: function (obj) {
            var curleft = 0,
                curtop = 0;
            if (obj.offsetParent) {
                curleft = obj.offsetLeft;
                curtop = obj.offsetTop;
                while ((obj.offsetParent)) {
                    obj = obj.offsetParent;
                    curleft = curleft + obj.offsetLeft;
                    curtop = curtop + obj.offsetTop;
                }
            }
            return { x: curleft, y: curtop };
        },
        getWindowInnerSize: function () {
            var db = doc.body,
                de = doc.documentElement,
                result;
            if (typeof (window.innerWidth) === 'number') {
            // Standards compliant browsers.
                result = { x: window.innerWidth, y: window.innerHeight };
            } else if (de && (de.clientWidth || de.clientHeight)) {
            // IE 6-8 standards mode.
                result = { x: de.clientWidth, y: de.clientHeight };
            } else if (db && (db.clientWidth || db.clientHeight)) {
            // IE 4 and IE 5-8 quirks mode.
                result = { x: db.clientWidth, y: db.clientHeight };
            } else {
                result = { x: 0, y: 0 };
            }
            return result;
        },
        getScrollPosition: function () {
            var db = doc.body || null,
                de = doc.documentElement || null,
                result = {},
                x = 0,
                y = 0;
            if (db && (db.scrollLeft || db.scrollTop)) {
            // Standards compliant browsers.
                result = { x: db.scrollLeft, y: db.scrollTop };
            } else if (de && (de.scrollLeft || de.scrollTop)) {
            // IE 6.
                result = { x: de.scrollLeft, y: de.scrollTop };
            } else {
                result = { x: x, y: y };
            }
            return result;
        },
        getStyle: function (element, style) {
            var css,
                camelStyle = CARPE.camelize(style),
                value = element.style[camelStyle],
                view = doc.defaultView;
            if (!value) {
                if (view && view.getComputedStyle) {
                    css = view.getComputedStyle(element, null);
                    value = css ? css[camelStyle] : null;
                } else if (element.currentStyle) {
                    value = element.currentStyle[camelStyle];
                }
            }
            return value;
        },
        camelize: function (str, splitter) {
            var parts = str.split(splitter || '-'),
                len = parts.length,
                camel = parts[0],
                i;
            if (len > 1) {
                for (i = 1; i < len; i += 1) {
                    camel += parts[i].charAt(0).toUpperCase() +
                            parts[i].substring(1);
                }
            }
            return camel;
        },
        touchListener: function(e) {
            var touches = e.changedTouches,
                first = touches[0],
                type = '',
                simulated = document.createEvent("MouseEvent"),
                bubble = true,
                cancelable = true,
                view = window,
                clicks = 1,
                ctrl = false,
                alt = false,
                shift = false,
                meta = false,
                button = 0,
                relatedTarget = null;
            switch(e.type) {
            case 'touchstart': type = 'mousedown'; break;
            case 'touchmove': type = 'mousemove'; break;        
            case 'touchend': type = 'mouseup'; break;
            default: return;
            }
            simulated.initMouseEvent(type, bubble, cancelable, view, clicks, 
                    first.screenX, first.screenY, 
                    first.clientX, first.clientY, ctrl, 
                    alt, shift, meta, button, relatedTarget);
            first.target.dispatchEvent(simulated);
            e.preventDefault();
        },
        stopEvent: function (e) {
            e = e || window.event;
            var stop = false;
            if (e) {
                if (e && e.preventDefault) {
                    e.preventDefault();
                    e.stopPropagation();
                } else {
                    e.returnValue = false;
                    e.cancelBubble = true;
                }
                stop = true;
            }
            return stop;
        },
        addEventListener: (function () {
            var add = function add(obj, type, func) { // W3C.
                    obj.addEventListener(type, func, false);
                },
                addIE = function (obj, type, func) {
                    obj.attachEvent('on' + type, func);
                };
            if (!window.addEventListener && doc.attachEvent) { // IE.
                add = addIE;
            }
            return add;
        }()),
        removeEventListener: (function () {
            var remove = function (obj, type, func) { // W3C.
                    obj.removeEventListener(type, func, false);
                },
                removeIE = function (obj, type, func) {
                    if (obj.detachEvent) {
                        obj.detachEvent('on' + type, func);
                    }
                };
            if (doc.detachEvent) { // IE.
                remove = removeIE;
            }
            return remove;
        }())
    };
}());
//  End of CARPE Common library.


//  Start of CARPE Slider script.
(function () {
    'use strict';
    var CARPE = window.CARPE,
        doc = window.document;
    CARPE.Sliders = {
        version: '2.0.8',
        elements: [], // Array that holds all slider elements in a page.
        objects: [], // Array that holds all slider objects in a page.

        // Method 'init': Loops through slider elements and creates the
        // corresponding slider objects. 
        init: function () {
            var i,
                sliderClass = CARPE.Slider.prototype,
                elements = CARPE.getElementsByClass(sliderClass.sliderClass),
                len = elements.length,
                classPrefix = sliderClass.panelClass + '-';
            CARPE.Sliders.elements = elements;
            for (i = 0; i < len; i = i + 1) {
                if (!doc.getElementById(classPrefix + elements[i].id)) {
                    CARPE.Sliders.make(elements[i]);
                }
            }
            return;
        },
        make: function (elmnt, params, DOMLocation) {
            var slider = new CARPE.Slider(elmnt, params, DOMLocation);
            CARPE.Sliders.objects.push(slider);
            return slider;
        },
        setValue: function (id, value, prevent) {
            var i,
                sliders = CARPE.Sliders.objects,
                len = sliders.length;
            for (i = 0; i < len; i = i + 1) {
                if (sliders[i].id && (sliders[i].id === id)) {
                    sliders[i].setValue(value, prevent);
                }
            }
            return;
        },
        getValue: function (id) {
            var i,
                sliders = CARPE.Sliders.objects,
                len = sliders.length,
                value;
            for (i = 0; i < len; i = i + 1) {
                if (sliders[i].id && (sliders[i].id === id)) {
                    value = sliders[i].getValue();
                }
            }
            return value;
        }
    };
}());

/*  The 'Slider' class constructor: elmnt is a proper input element object
        or an id string. params is an object with the optional properties..
            orientation ('horizontal' or 'vertical'),
            disabled, ('true', 'yes', 'false', or 'no')
            from, (start value)
            to, (end value)
            min, (highest value, may be start value or end value)
            max, (lowest value, may be start value or end value)
            value (initial value, a number),
            position (number of pixels),
            size (number of pixels, default is 100),
            stops (number of stops that the slider can snap to, 0 is default
                and means no snapping),
            step (size of snapping steps),
            source (initial value from other form element, source element id),
            target (target element id, object or object property),
            decimals (integer),
            feedback (true or false, true means that snapping at the target
                controls snapping for the slider)
*/
var CARPE = window.CARPE;
CARPE.Slider = function (elmnt, params, DOMLocation) {
    'use strict';
    // The original (X)HTML slider element:
    var startElmnt = {},
        doc = window.document;

    (function (o) { // Find and/or create a Carpe slider input element. 
        var actualElement,
            id,
            idBase,
            options,
            startClass,
            stops;
        if (elmnt) {
            if (typeof elmnt === 'string') {
                id = elmnt;
                actualElement = doc.getElementById(elmnt) || null;
            } else if (elmnt.nodeType === 1) {
                actualElement = elmnt;
                options = elmnt.options || null;
                stops = options ? (options.length || undefined) : undefined;
            }
        }
        // Create the element if it's not there.
        startElmnt = actualElement || doc.createElement('input');

        // The slider ID & name:
        if (!startElmnt.id && !id) {
            id = 0;
            idBase = o.sliderClass + '-' + o.idPrefix + '-';
            while (doc.getElementById(idBase + id)) {
                id += 1;
            } // Find a unique id.
            startElmnt.id = idBase + id;
        }
        o.id = startElmnt.id || id;
        o.attributes = startElmnt.attributes || [];
        o.name = startElmnt.name;

        // The Slider Classname:
        if (startElmnt.className) {
            startClass = startElmnt.className;
            o.className = (startClass.indexOf(o.sliderClass) === -1) ?
                    (startClass + ' ' +
                    o.sliderClass) : startElmnt.className;
        } else {
            o.className = o.sliderClass;
        }
    }(this));

    (function (o) { // Default values
        o.disabled = false;
        o.orientation = 'horizontal';
        o.position = 0;
        o.value = null;
        o.size = 100;
        o.from = 0;
        o.to = 1;
        o.min = null;
        o.max = null;
        o.unit = 'px';
        o.feedback = false;
        o.zerofill = false;
        o.targets = [];
        o.slit = {};
        o.panel = {};
        o.knob = {};
        o.step = 0;
        o.stops = 0;
        o.decimals = 14;
    }(this));

    /* The local function assignToProps: takes an object of name/value pairs,
       and tries to assign values to the slider properties regardless of the
       method of user customization. */
    function assignToProps(list, target) {
        var name,
            val;
        for (name in list) {
            if (Object.prototype.hasOwnProperty.call(list, name)) {
                val = list[name];
                switch (name) {
                case 'disabled':
                    target[name] = true;
                    break;
                case 'orientation':
                case 'target':
                case 'source':
                    target[name] = val;
                    break;
                case 'size':
                case 'position':
                    target[name] = parseInt(val, 10) || target[name];
                    break;
                case 'decimals':
                    target[name] = parseInt(val, 10);
                    break;
                case 'from':
                case 'to':
                case 'min':
                case 'max':
                    target[name] = parseFloat(val, 10);
                    break;
                case 'value':
                    target[name] = parseFloat(val, 10) || 0;
                    break;
                case 'slit':
                    target[name] = (val === 'false' || val === 'no') ?
                            false : target[name];
                    break;
                case 'stops':
                    target[name] = Math.abs(parseInt(val, 10));
                    target.step = 0;
                    break;
                case 'step':
                    target[name] = Math.abs(parseFloat(val, 10));
                    target.stops = 0;
                    break;
                case 'targets':
                    target[name] = val.toString().split(/\s+/);
                    break;
                case 'feedback':
                case 'zerofill':
                    target[name] = ((val === 'true') || (val === 'yes')) ?
                            true : false;
                    break;
                default:
                    break;
                }
            }
        }
    }

    (function (o) { // Handle user supplied properties.
        var atts = o.attributes,
            classNames = o.className ? o.className.split(/\s+/) : [],
            i,
            len,
            name,
            prefix = o.attributePrefix,
            props = {};
        // Properties supplied as constructor arguments:
        if (params) {
            CARPE.transfer(params, props);
        }
        // Properties supplied with the class attribute (recommended with
        // HTML 4 and XHTML):
        len = classNames.length;
        for (i = 0; i < len; i += 1) {
            name = classNames[i].split('-')[0];
            props[name] = classNames[i].substring(name.length + 1,
                    classNames[i].length);
        }
        // Properties supplied with custom carpe attributes
        // data-carpe-...=".." (recommended with HTML 5):
        len = atts.length;
        for (i = 0; i < len; i += 1) {
            if (atts[i].nodeName.indexOf(prefix) >= 0) { // Remove 'data-carpe-'
            // prefix if present.
                props[atts[i].nodeName.replace(prefix, '')] = atts[i].nodeValue;
            } else {
                props[atts[i].nodeName] = atts[i].nodeValue;
            }
        }
        assignToProps(props, o);
    }(this));

    (function (o) { // Initial values for slider movement limitation:
        var temp;
        if (o.max !== null) {
            o.to = o.max;
        } else {
            o.max = Math.max(o.to, o.from);
        }
        if (o.min !== null) {
            o.from = o.min;
        } else {
            o.min = Math.min(o.to, o.from);
        }
        if (o.max < o.min) {
            temp = o.max;
            o.max = o.min;
            o.min = temp;
        }
        o.vertical = (o.orientation === 'vertical');
        o.range = o.to - o.from;
        o.scale = (o.size === 0) ? 0 : o.range / o.size;
        o.xMax = !o.vertical ? o.size : null;
        o.yMax = o.vertical ? o.size : null;
        o.position = (o.vertical && (o.position === 0)) ? o.size : o.position;
        o.position = o.position > o.size ? o.size : o.position;
        if (o.value === null) {
            o.value = (o.size > 0) ? (o.position / o.size) *
                    o.range + o.from : 0;
        } else {
            o.value = (o.value > o.max) ? o.max :
                    ((o.value < o.min) ? o.min : o.value);
            o.position = o.value < o.max ? Math.round((o.value - o.min) /
                o.range * o.size) : o.size;
        }
        o.x = o.y = o.position;
        o.snapping = (o.stops > 0) || (Math.abs(o.step) > 0);
        o.stops = (o.stops > o.size) ? 0 : o.stops;
        if (Math.abs(o.stops) < 2) {
            if (o.step === 0) {
                o.step = (o.size === 0) ? 0 : o.range / o.size;
                o.pxStep = (o.range < 0) ? -1 : 1;
            } else {
                o.pxStep = o.scale ? o.step / o.scale : 1;
            }
        } else {
            if (o.step === 0) {
                o.step = o.range / (o.stops - 1);
                o.pxStep = o.range < 0 ? o.size / (1 - o.stops) :
                        o.size / (o.stops - 1);
            } else {
                o.pxStep = o.scale ? o.step / o.scale : 1;
            }
        }
        o.lastStep = o.size - Math.floor(o.size / o.pxStep) * o.pxStep;
        o.lastStep = (o.pxStep < 0) ? -o.lastStep : o.lastStep;
    }(this));

    (function (o) { // Slider targets:
        var el,
            i,
            j,
            len,
            parts;
        if (o.target) {
            o.targets.push(o.target);
        }
        len = o.targets.length;
        if (len) {
            for (i = 0; i < len; i = i + 1) {
                el = doc.getElementById(o.targets[i]);
                if (el) {
                    o.targets[i] = doc.getElementById(o.targets[i]);
                } else {
                    parts = o.targets[i].split('.');
                    if (doc.getElementById(parts[0]) || window[parts[0]]) {
                        o.targets[i] = doc.getElementById(parts[0]) ||
                                window[parts[0]];
                        if (parts.length > 1) {
                            for (j = 1; j < parts.length; j += 1) {
                                if (!o.targets[i][parts[j]]) {
                                    o.targets[i][parts[j]] = {};
                                }
                                o.targets[i] = o.targets[i][parts[j]];
                            }
                        }
                    }
                }
            }
        }
    }(this));

    (function (o) { // Parent element and the slider's DOM location:
        var getId = doc.getElementById,
            loc = DOMLocation,
            node;
        if (startElmnt.parentNode) {
            o.parent = startElmnt.parentNode;
        } else {
            o.parent = doc.forms[0] || doc;
            if (loc) {
                if (loc.parent) {
                    o.parent = (typeof loc.parent === 'string') ?
                            getId(loc.parent) : loc.parent;
                }
                if (loc.before) {
                    o.before = (typeof loc.before === 'string') ?
                            getId(loc.before) : loc.before;
                    o.parent = o.before.parentNode;
                }
                if (loc.after) {
                    o.after = (typeof loc.after === 'string') ?
                            getId(loc.after) : loc.after;
                    o.parent = o.after.parentNode;
                }
            }
            // The slider's location in the DOM
            if (!o.before && !o.after) {
                o.parent.appendChild(startElmnt);
            } else if (o.before) {
                o.parent.insertBefore(startElmnt, o.before);
            } else if (o.after) {
                node = o.after.nextSibling;
                while (node.nodeType !== 1) {
                    node = node.nextSibling;
                }
                o.parent.insertBefore(startElmnt, node);
            }
        }
    }(this));

    (function (o) { // The new hidden value element:
        var el = doc.createElement('input');
        el.setAttribute('type', 'hidden');
        el.setAttribute('name', o.name);
        el.className = o.className;
        o.parent.insertBefore(el, startElmnt);
        o.parent.removeChild(startElmnt);
        el.disabled = startElmnt.disabled;
        el.id = o.id;
        el.knob = o.knob;
        el.panel = o.panel;
        el.slit = o.slit;
        el.setValue = function () { o.setValue.call(this); };
        o.valueElmnt = el;
    }(this));

    (function (o) { // The slider panel:
        var noAction = 'javascript: void 0;';
        o.panel = doc.createElement('a');
        o.panel.setAttribute('href', noAction);
        o.panel.setAttribute('tabindex', '0');
        o.panel.style.cssText = startElmnt.style.cssText; // Copy styles from
        // the user supplied input element.
        o.parent.insertBefore(o.panel, o.valueElmnt);
        o.panel.className = o.panelClass + ' orientation-' +
            o.orientation; // + ' target-' + o.target.id;
        o.panel.id = o.panelClass + '-' + o.id;
    }(this));

    (function (o) { // The slider knob:
        var knob = doc.createElement('div'),
            style = CARPE.getStyle,
            width,
            height;
        knob.className = o.knobClass;
        knob.id = o.knobClass + '-' + o.id;
        o.panel.appendChild(knob);
        knob.width = parseInt(style(knob, 'width'), 10);
        knob.height = parseInt(style(knob, 'height'), 10);
        if (!o.vertical) {
            width = o.size + knob.width;
            if (!window.opera) {
                width += parseInt(style(knob, 'border-left-width'), 10) +
                        parseInt(style(knob, 'border-right-width'), 10);
            }
            knob.size = width;
            o.panel.style.width = width.toString() + o.unit;
        } else {
            height = o.size + knob.height;
            if (!window.opera) {
                height += parseInt(style(knob, 'border-top-width'), 10) +
                        parseInt(style(knob, 'border-bottom-width'), 10);
            }
            knob.size = height;
            o.panel.style.height = height.toString() + o.unit;
        }
        o.knob = knob;
        o.knob.location = o.bind(CARPE.position, o.knob);
    }(this));

    (function (o) { // The slider slit:
        var style = CARPE.getStyle,
            slit;
        if (o.slit) {
            slit = doc.createElement('div');
            slit.className = o.slitClass;
            slit.id = o.slitClass + '-' + o.id;
            o.panel.appendChild(slit);
            if (!o.vertical) {
                slit.style.width = (o.size + o.knob.width -
                parseInt(style(slit, 'border-left-width'), 10) -
                parseInt(style(slit,
                        'border-right-width'), 10)).toString() + o.unit;
                if (window.opera) {
                    slit.style.width = (parseInt(slit.style.width, 10) -
                            parseInt(style(o.knob, 'border-left-width'), 10) -
                            parseInt(style(o.knob,
                            'border-right-width'), 10)).toString() + o.unit;
                }
            } else {
                slit.style.height = (o.size + o.knob.height -
                parseInt(style(slit, 'border-top-width'), 10) -
                parseInt(style(slit, 'border-bottom-width'), 10)).toString() +
                    o.unit;
                if (window.opera) {
                    slit.style.height = (parseInt(slit.style.height, 10) -
                            parseInt(style(o.knob, 'border-top-width'), 10) -
                            parseInt(style(o.knob,
                            'border-bottom-width'), 10)).toString() + o.unit;
                }
            }
            o.slit = slit;
        }
    }(this));

    (function (o) { // Event handlers:
        CARPE.addEventListener(o.knob, 'mousedown', o.bind(o.start, o));
        CARPE.addEventListener(o.panel, 'mousedown', o.bind(o.jump, o));
        CARPE.addEventListener(o.knob, 'touchstart', CARPE.touchListener);
        CARPE.addEventListener(o.panel, 'touchstart', CARPE.touchListener);
        o.mouseMoveListener = o.bind(o.move, o);
        o.mouseUpListener = o.bind(o.mouseUp, o);
        o.panel.onblur = o.bind(o.makeBlurred, o);
        if (window.opera) {
            o.panel.onkeypress = o.bind(o.keyHandler, o);
        } else {
            o.panel.onkeydown = o.bind(o.keyHandler, o);
        }
    }(this));

    (function (o) { // Move slider knob to initial position and set value:
        if (o.source && doc.getElementById(o.source)) {
            o.setValue(doc.getElementById(o.source).value);
        }
        o.snap();
        o.enabled(o.enabled());
    }(this));
};

//  The Slider class:
CARPE.Slider.prototype = {
    idPrefix: 'auto', // Prefix used for auto-generated slider element IDs.
    sliderClass: 'carpe-slider', // The class name for the silders.
    panelClass: 'carpe-slider-panel', // CSS selector for the slider panel.
    slitClass: 'carpe-slider-slit', // CSS selector for the slider slit.
    knobClass: 'carpe-slider-knob', // CSS selector for the slider knob.
    attributePrefix: 'data-carpe-', // Prefix for Carpe custom attributes.

    // Class method 'bind': binds a function or method to an
    // invocation context.
    bind: function (method, context) {
        'use strict';
        return function () {
            return method.apply(context, arguments);
        };
    },
    // Class method 'makeFocused': handles added focus.
    makeFocused: function () {
        'use strict';
        return this;
    },
    // Class method 'makeBlurred': handles removed focus.
    makeBlurred: function () {
        'use strict';
        if (this.slit) {
            this.slit.className = this.slitClass;
        }
        return this;
    },
    // Class method 'keyHandler': handles arrow key input for slider.
    keyHandler: function (evnt) {
        'use strict';
        evnt = evnt || window.event; // Get the key event.
        var LEFT = 37, // Right key is 38.
            DOWN = 40, // Up key is 39.
            increment = 1,
            key,
            result = false;
        if (evnt && !this.disabled) {
            key = evnt.which || evnt.keyCode; // Get the key code.
            if ((key === LEFT) || (key === DOWN)) {
                increment = -increment;
            }
            if ((key < LEFT) || (key > DOWN)) { // Not arrow keys (37-40).
                increment = 0;
                result = true;
            } else {
                increment = this.vertical ? -increment : increment;
                increment = (this.range < 0) ? -increment : increment;
                this.moveInc(increment);
            }
            return result;
        }
    },
    // Class method 'moveInc': moves slider a number of steps (pixels if no
    // 'steps' attribute is specified).
    moveInc: function (increment) {
        'use strict';
        var step = this.pxStep,
            last = this.lastStep,
            reverse = (this.range < 0) ? true : false,
            back = (increment < 0) ? true : false,
            start = (this.position === 0) ? true : false,
            end = (this.position === this.size) ? true : false,
            vertical = this.vertical;
        if (last) {
            if (end && !vertical) {
                step = (reverse && !back) ? -last : step;
                step = (!reverse && back) ? last : step;
            }
            if (start && vertical) {
                step = (reverse && back) ? -last : step;
                step = (!reverse && !back) ? last : step;
            }
        }
        this.position += (increment * step);
        return this.update();
    },
    // Class method 'snap': moves slider to fixed positions specified by
    // 'stops' or 'step'.
    snap: function () {
        'use strict';
        if (this.stops > 1) { // HTML 4 and XHTML style.
            this.position = parseInt(this.size * Math.round(this.position *
                (this.stops - 1) / this.size) / (this.stops - 1), 10);
        } else if (this.step > 0) { // HTML5 style.
            if ((this.size - this.position) < (this.pxStep / 2)) {
                this.position = this.size;
            } else {
                this.position = parseInt(Math.round(this.scale *
                        this.position / this.step) *
                        this.step / this.scale, 10);
            }
        }
        return this.update();
    },
    // Class method 'mouseUp': handles the end of the sliding process.
    mouseUp: function () {
        'use strict';
        var remove = CARPE.removeEventListener,
            doc = window.document;
        this.sliding = false;
        this.snap();
        remove(doc, 'touchend', CARPE.touchListener);
        remove(doc, 'mouseup', this.mouseUpListener);
        remove(doc, 'touchmove', CARPE.touchListener);
        remove(doc, 'mousemove', this.mouseMoveListener);
        if (this.slit) {
            this.slit.className = this.slitClass;
        }
        return this;
    },
    // Class method 'start': handles the start of the sliding process.
    start: function (evnt) {
        'use strict';
        var evnt = evnt || window.evnt,
            add = CARPE.addEventListener,
            doc = window.document,
            v = this.vertical;
        if (this.enabled()) {
            this.startOffset = (!v) * (this.x - evnt.screenX) + v * (this.y -
                    evnt.screenY);
            this.panel.focus();
            this.sliding = true;
            add(doc, 'touchmove', CARPE.touchListener); // Start the
            // action if the finger is dragged on touch screens.
            add(doc, 'mousemove', this.mouseMoveListener); // Start the
            // action if the mouse is dragged.
            add(doc, 'touchend', CARPE.touchListener); // Stop sliding.
            add(doc, 'mouseup', this.mouseUpListener); // Stop sliding.
        }
        CARPE.stopEvent(evnt);
        return this.enabled();
    },
    // Class method 'jump': handles an instant movement of the
    // slider when user clicks on the panel.
    jump: function (evnt) {
        'use strict';
        var evnt = evnt || window.event, // Get the mouse event causing the
        // slider activation.
            scroll,
            pos,
            border = {},
            getStyle = CARPE.getStyle;
        CARPE.stopEvent(evnt);
        if (!this.disabled) {
            scroll = CARPE.getScrollPosition();
            pos = CARPE.getPos(this.knob);
            border = {
                x: parseInt(getStyle(this.knob, 'border-left-width') +
                        getStyle(this.knob, 'border-right-width'), 10),
                y: parseInt(getStyle(this.knob, 'border-top-width') +
                    getStyle(this.knob, 'border-bottom-width'), 10)
            };
            if (this.vertical) { // Move slider to new vertical position.
                this.position = evnt.clientY - (pos.y - scroll.y - this.y +
                    parseInt((border.y + this.knob.height / 2), 10));
            } else { // Move slider to new horizontal position.
                this.position = evnt.clientX - (pos.x - scroll.x - this.x +
                    parseInt((border.x + this.knob.width / 2), 10));
            }
            this.update();
            this.start(evnt);
        }
        return this;
    },
    // Class method 'move': handles the movement of the slider while dragging.
    move: function (evnt) {
        'use strict';
        var evnt = evnt || window.event,
            result = true;
        if (this.sliding) {
            this.position = this.startOffset + this.vertical * evnt.screenY +
                (!this.vertical) * evnt.screenX;
            this.update();
            result = false;
        }
        return result;
    },
    // Class method 'pos': Moves the slider to a new pixel position and returns
    // the slider object.
    pos: function () {
        'use strict';
        var position = this.position,
            pos = {},
            dir = this.vertical ? 'y' : 'x';
        this.position = this[dir] = (position > this.size) ? this.size :
                ((position < 0) ? 0 : position);
        pos[dir] = this[dir];
        this.knob.location(pos);
        return this;
    },
    // Class method 'val': Calculates and sets the value of the hidden input
    // element, and the sliders value property and returns the slider object.
    val: function () {
        'use strict';
        var v = this.vertical,
            dir = v ? 'y' : 'x',
            val = this[dir] * (1 - 2 * v) * this.scale + this.from * (!v) +
                    this.to * v;
        val = (this.snapping && (this.size !== this.position)) ?
                Math.round(val / this.step) * this.step : val;
        this.value = CARPE.roundToDec(val, this.decimals, this.zerofill);
        this.valueElmnt.value = this.value;
        return this;
    },
    // Class method 'getValue': Gets value of the hidden slider input element.
    // Intended as a 'public' method for user scripts.
    getValue: function () {
        'use strict';
        return this.valueElmnt.value;
    },
    // Class method 'setValue': Sets value and positions knob accordingly.
    // Intended as a 'public' method for user scripts, and when a display
    // element publishes a feedback.
    setValue: function (value, prevent) {
        'use strict';
        var v = this.vertical;
        this.position = parseInt((value * (+!v) - this.from * (+!v) + this.to *
                v - value * v) / this.scale, 10);
        return this.update(prevent);
    },
    // Class method 'update': Updates the slider GUI with current
    // property values.
    update: function (prevent) {
        'use strict';
        this.pos().val();
        if (!prevent) {
            this.updateTargets();
        }
        if ((prevent !== true) && (prevent > 0)) {
            this.updateTargets(prevent);
        }
        return this;
    },
    // Class method 'enabled': Updates (from DOM), sets or gets
    // disabled status. 
    enabled: function (enabled) {
        'use strict';
        var dis = 'disabled',
            disabledAttr = this.valueElmnt.disabled,
            doc = window.document,
            panelAttr,
            panelTabAttr,
            result,
            tab = 'tabindex';
        this.disabled = disabledAttr; // Assume getter.
        result = !this.disabled;
        if (enabled !== undefined) { // If setter.
            this.disabled = !enabled;
            if (this.disabled) {
                this.valueElmnt.disabled = true;
                panelAttr = doc.createAttribute(dis);
                panelTabAttr = doc.createAttribute(tab);
                panelAttr.value = dis;
                panelTabAttr.value = '-1';
                this.panel.setAttributeNode(panelAttr);
                this.panel.tabIndexTemp = this.panel.attributes[tab];
                this.panel.setAttributeNode(panelTabAttr);
            } else {
                this.valueElmnt.disabled = false;
                this.panel.removeAttribute(dis);
                this.panel.removeAttribute(tab);
                if (this.panel.tabIndexTemp) {
                    this.panel.setAttributeNode(this.panel.tabIndexTemp);
                }
            }
            result = this; // If setter return the slider object. 
        }
        return result; // If getter return true (enabled) or false (disabled).
    },
    // Class method 'updateTargets': Sets the value for the target element.
    updateTargets: function (to) {
        'use strict';
        var i,
            len,
            val = this.value,
            targets = this.targets || null,
            t;
        if (targets && (targets.length > 0)) {
            len = to || targets.length;
            for (i = 0; i < len; i += 1) {
                t = targets[i];
                if (t.setValue && CARPE.isFunction(t.setValue)) {
                    t.setValue(val);
                } else if (t.constructor.toString().
                        toLowerCase().indexOf('input') >= 0) {
                    t.value = val;
                } else if (CARPE.isFunction(t)) {
                    setTimeout(function () { t(val); }, 50);
                } else if (t.nodeType && (t.nodeType === 1)) {
                    t.innerHTML = val;
                } else {
                    t = val;
                }
            }
        }
        return this;
    }
};
//  Make sure the document is loaded before the slider initiation starts:

CARPE.domLoaded(CARPE.Sliders.init);

//  End of script.