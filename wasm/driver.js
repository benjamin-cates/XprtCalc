const JSFailMessage = setTimeout(function () {document.querySelector(".hide_on_next_xpr").innerHTML = "An error has occured on startup. It's possible that the browser does not support the features required for this program. Please report <a href='https://github.com/benjamin-cates/XprtCalc/issues'>here</a>.";}, 2000);
var Module = {};
async function onProgramInitialized() {
    update_highlight("");
    //parse url arguments
    const params = new URLSearchParams(window.location.search);
    //init url arguments
    if(params.has("init")) {
        //Hide welcome message
        document.querySelectorAll(".hide_on_next_xpr").forEach(x => x.remove());
        //Split by semicolons and run each as a line
        const init = params.get("init").split(";");
        console.log(init);
        for(let i = 0; i < init.length; i++)
            await run_line(init[i]);
    }
    window.clearTimeout(JSFailMessage);
};
async function run_line(str, coloredInput = "") {
    if(coloredInput == "")
        coloredInput = Module.highlightLine ? Module.highlightLine(str) : str.replace(/</g, "&lt;").replace(/>/g, "&gt;");
    let out = document.createElement("div");
    out.className = "output_element";
    try {
        await loadArbIfNecessary(str);
    } catch(e) {
        out.innerHTML = coloredInput + "<br><Ce>Error:</Ce> failed to load arb library.";
    }
    try {
        if(Module.runLineWithColor) {
            let output = Module.runLineWithColor(str).replace(/\n/g, "<br>");
            if(output.includes("<Ce>Error:") || output == "")
                out.classList.add("hide_on_next_xpr");
            out.innerHTML = coloredInput + "<br>" + output;
        }
        else {
            out.classList.add("hide_on_next_xpr");
            out.innerHTML = "<Ce>Error:</Ce> page not finished loading";
        }
    }
    catch(e) {
        out.innerHTML = coloredInput + "<br><Ce>Error:</Ce> unrecognizable error, please report to <a href='https://github.com/benjamin-cates/XprtCalc/issues'>https://github.com/benjamin-cates/XprtCalc/issues</a> with more information.";
    }
    document.querySelector("#input_element").value = "";
    document.querySelector("#highlight_output").innerHTML = "";
    document.querySelector("#output").prepend(out);
}
const textArea = document.querySelector("#input_element");

function textAreaKeydown(event) {
    if(event.key == "Enter" && !event.shiftKey) {
        //Hide previous result if it is an error or non-returning command
        document.querySelectorAll(".hide_on_next_xpr").forEach(x => x.remove());
        if(textArea.value.length == 0)
            textArea.value = "";
        else {
            let str = textArea.value.replace(/\n/g, "");
            run_line(str, document.querySelector("#highlight_output").innerHTML);
        }
        if(event.preventDefault) event.preventDefault();
    }
}
textArea.addEventListener("focus", event => {
    keyboard.shown = keyboard.enabled;
    if(keyboard.shown) keyboard.element.classList.add("shown");
});
window.addEventListener("click", event => {
    //Close keyboard if somewhere else is clicked
    if(keyboard.shown) {
        //If click is within bounding rectangle of keyboard, ignore it
        if(event.clientY > window.innerHeight - keyboard.element.clientHeight) {
            if(event.clientX > window.innerWidth / 2 + keyboard.element.clientWidth / 2);
            else if(event.clientX < window.innerWidth / 2 - keyboard.element.clientWidth / 2);
            else return;
        }
        //Ignore if focus is in textArea
        if(document.activeElement == textArea) return;
        //Hide keyboard
        keyboard.element.classList.remove("shown");
        keyboard.shown = false;
    }
});


function update_highlight(str) {
    this.output = document.querySelector("#highlight_output");
    this.text = str;
    textArea.style.height = "1px";
    document.querySelector("#input").style.height = (textArea.scrollHeight) + "px";
    textArea.style.height = (textArea.scrollHeight) + "px";
    if(Module.highlightLine) this.output.innerHTML = Module.highlightLine(this.text);
    else this.output.innerText = this.text;
}
window.onload = _ => {
    if(navigator.userAgentData) keyboard.enabled = navigator.userAgentData.mobile;
    else {
        let userAgent = navigator.userAgent;
        if(userAgent.includes("Android")) keyboard.enabled = true;
        else if(userAgent.includes("iPhone")) keyboard.enabled = true;
    }
    if(keyboard.enabled) {keyboard.enabled = false; document.querySelector("#keyboard_open").click();}
    textArea.focus();
    resize();
    window.addEventListener("resize", resize);
    keyboardConstructor.construct();
}

var panelData = {
    isOpen: false,
    page: "help"
};

function openHelpPanel(open) {
    if(open == false)
        panelData.isOpen = false;
    else
        panelData.isOpen = true;
    resize();
}

function resize() {
    let width = window.innerWidth;
    let height = window.innerHeight;
    if(panelData.isOpen) {
        document.querySelector("#panel").classList.add("open");
        if(width < 700) {
            document.querySelector("#panel").classList.add("mobile");
            document.querySelector("#io").classList.remove("pushed");
        }
        else {
            document.querySelector("#panel").classList.remove("mobile");
            document.querySelector("#io").classList.add("pushed");
        }
    }
    else {
        document.querySelector("#panel").classList.remove("open");
        document.querySelector("#io").classList.remove("pushed");
    }
    if(width < 700) keyboard.element.classList.add("mobile");
    else keyboard.element.classList.remove("mobile");
}
function helpSearch(event, queryBox) {
    if(event.key == "Enter") {
        document.querySelector("#help_page").classList.remove("shown");
        document.querySelector("#search_results").style.display = "block";
        let data = JSON.parse(Module.query(queryBox.value.toString()));
        let queryResults = "";
        for(let i = 0; i < data.length; i++) {
            queryResults += "<div class='search_result' onclick='openHelp(" + data[i].id + ")'>" + data[i].name + "</div>";
        }
        if(data.length == 0)
            queryResults += "No results found for " + queryBox.value;
        document.querySelector("#search_results").innerHTML = queryResults;
        queryBox.value = "";
        document.querySelector("#search_results").style.display = "block";
        document.querySelector("#help_page_front").style.display = "none";
    }
    if(queryBox == "!click") {
        document.querySelector("#help_page").classList.remove("shown");
        document.querySelector("#search_results").style.display = "none";
        document.querySelector("#help_page_front").style.display = "block";
    }
}
let helpPageHistory = [];
function openHelp(token) {
    //Handle back button
    if(token == "back") {
        //Go to front page if it was previous
        if(helpPageHistory.length <= 1) {
            helpSearch({key: "none"}, "!click");
            helpPageHistory = [];
            return;
        }
        //Pop current page
        helpPageHistory.pop();
        //Open last page
        token = helpPageHistory.pop();
    }
    //Open help panel
    openHelpPanel(true);
    //Hide search results and front page
    document.querySelector("#search_results").style.display = "none";
    document.querySelector("#help_page_front").style.display = "none";
    let id = token;
    //Get id of help page if token is a string
    if(typeof token == "string") id = JSON.parse(Module.query(token))[0].id;
    helpPageHistory.push(id);
    //Open help page
    document.querySelector("#help_page").innerHTML = Module.helpPage(id);
    document.querySelector("#help_page").classList.add("shown");
}
function insertAtCursor(myField, myValue) {
    //IE support
    if(document.selection) {
        myField.focus();
        sel = document.selection.createRange();
        sel.text = myValue;
    }
    //MOZILLA and others
    else if(myField.selectionStart || myField.selectionStart == '0') {
        var startPos = myField.selectionStart;
        var endPos = myField.selectionEnd;
        myField.value = myField.value.substring(0, startPos)
            + myValue
            + myField.value.substring(endPos, myField.value.length);
        myField.selectionStart = startPos + myValue.length;
        myField.selectionEnd = startPos + myValue.length;
    } else {
        myField.value += myValue;
    }
}
const keyboard = {
    element: document.querySelector("#keyboard"),
    shift: false,
    default: false,
    active: false,
    interval: null,
    intervalKey: "",
    intervalDelay: null,
    clearInterval: _ => {
        clearInterval(keyboard.interval);
        clearTimeout(keyboard.intervalDelay);
        keyboard.interval = null;
        keyboard.intervalDelay = null;
        keyboard.intervalKey = "";
    },
    currentPage: "page1",
    svg: {
        rightArrow: "<svg width='1em' height='1em' viewbox='0 0 13 13'> <path d='M 3 3 L 3 2 L 4 1 L 5 1 L 10 6 L 10 7 L 5 12 L 4 12 L 3 11 L 3 10 L 6 7 L 6 6 L 3 3'></path> </svg>",
        leftArrow: "<svg width='1em' height='1em' viewbox='0 0 13 13'> <g transform='translate(-2,-2)rotate(180,7.5,7.5)'><path d='M 3 3 L 3 2 L 4 1 L 5 1 L 10 6 L 10 7 L 5 12 L 4 12 L 3 11 L 3 10 L 6 7 L 6 6 L 3 3'></path> </g> </svg>",
        upArrow: "<svg width='1em' height='1em' viewbox='0 0 13 13'> <g transform='translate(0,-2)rotate(270,7.5,7.5)'><path d='M 3 3 L 3 2 L 4 1 L 5 1 L 10 6 L 10 7 L 5 12 L 4 12 L 3 11 L 3 10 L 6 7 L 6 6 L 3 3'></path> </g> </svg>",
        downArrow: "<svg width='1em' height='1em' viewbox='0 0 13 13'> <g transform='translate(-1,2)rotate(90,7.5,7.5)'><path d='M 3 3 L 3 2 L 4 1 L 5 1 L 10 6 L 10 7 L 5 12 L 4 12 L 3 11 L 3 10 L 6 7 L 6 6 L 3 3'></path> </g> </svg>",
        backspace: "<svg width='1.5em' height='1em' viewbox='0 -1 20 14'><path d='M 2 7 L 7 2 L 17 2 L 17 3 L 7.5 3 L 3.5 7 L 7.5 11 L 16 11 L 16 3 L 17 3 L 17 12 L 7 12 L 2 7'/></svg>",
        backspaceFull: "<svg width='1.5em' height='1em' viewbox='0 -1 20 14'><path d='M 2 7 L 7 2 L 17 2 L 17 12 L 7 12 L 2 7'/></svg>",
        shift: "<svg width='1em' height='1.3em' style='margin:0 -0.3ch 0 -0.3ch' viewbox='2 0 8 8'><path d='M 3 5 L 6 2 L 9 5 L 7.8 5.2 L 7.8 8 L 4.2 8 L 4.2 5.2 L 3 5 L 4.8 4.8 L 4.8 7.4 L 7.2 7.4 L 7.2 4.8 L 7.9 4.66 L 6 2.9 L 4.1 4.66 L 4.8 4.8'/></svg>",
        shiftFull: "<svg width='1em' height='1.3em' style='margin:0 -0.3ch 0 -0.3ch' viewbox='2 0 8 8'><path d='M 3 5 L 6 2 L 9 5 L 7.8 5.2 L 7.8 8 L 4.2 8 L 4.2 5.2 L 3 5'/></svg>",
        enter: "E",
    }
};
window.addEventListener("pointerup", keyboard.clearInterval);
const keyboardConstructor = {
    construct: _ => {
        let out = "";
        for(let page = 1; page >= 0; page--) for(let shift = 0; shift < 2; shift++) {
            const keyList = keyboardConstructor.data[page][shift];
            out += `<div class='page key_page${page == 0 ? "1" : "2"} ${shift == 0 ? "no_shift" : "need_shift"}'>`;
            for(let i = 0; i < keyList.length; i++) {
                let k = "";
                let keyClass = "key";
                let otherAttributes = " onpointerout='keyboard.clearInterval()' onclick='textArea.focus()'";
                if(keyList[i] == "") {
                    out += "<div></div>";
                    continue;
                }
                if(typeof keyList[i] == "string") {
                    k = keyList[i];
                    otherAttributes += "onpointerdown='pressKey(this)'";
                }
                else {
                    k = keyList[i].k;
                    if(keyList[i].n) otherAttributes += `onpointerdown="pressKey('${keyList[i].n}')" title="${keyList[i].n}"`;
                    if(keyList[i].col || keyList[i].row) {
                        otherAttributes += "style='";
                        if(keyList[i].col) otherAttributes += "grid-column: " + keyList[i].col;
                        if(keyList[i].row) otherAttributes += ";grid-row: " + keyList[i].row;
                        otherAttributes += "'";
                    }
                }
                //Center row of qwerty keyboard
                if(i >= 20 && i <= 29 && page == 1) {
                    keyClass += " offset_key";
                }
                let col = k.substring(k.length - 1);
                out += `<button class='${keyClass}'${otherAttributes}><C${col}>${k.substring(0, k.length - 1)}</C${col}></button > `;

            }
            out += "</div>";
        }
        keyboard.element.innerHTML = out;
    },
    data: [
        [
            [
                "(b", ")b", "&lt;b", "&gt;b", "=o", "ansf",
                {k: `${keyboard.svg.leftArrow} t`, n: "left arrow"}, "7n", "8n", "9n", "/o", {k: `${keyboard.svg.backspace} c`, n: "backspace"},
                {k: `${keyboard.svg.rightArrow} t`, n: "right arrow"}, "4n", "5n", "6n", "*o", {k: "C/", n: "clear"},
                {k: `${keyboard.svg.shift} t`, n: "shift"}, "1n", "2n", "3n", "-o", {k: `${keyboard.svg.enter} /`, n: "enter", col: "6", row: "4/6"},
                {k: "ABCt", n: "abc"}, ",d", "0n", ".n", "+o"
            ],
            [
                "[b", "]b", "{b", "}b", "=o", "$f",
                {k: `${keyboard.svg.upArrow}t`, n: "up arrow"}, "7n", "8n", "9n", "\\t", {k: `${keyboard.svg.backspaceFull}c`, n: "backspace"},
                {k: `${keyboard.svg.downArrow}t`, n: "down arrow"}, "4n", "5n", "6n", "^o", {k: "C/", n: "clear"},
                {k: `${keyboard.svg.shiftFull}t`, n: "shift"}, "1n", "2n", "3n", "_t", {k: `${keyboard.svg.enter}/`, n: "enter", col: "6", row: "4/6"},
                {k: "ABCt", n: "abc"}, ",d", "0n", ".n", "+o"
            ],
        ], [
            [
                "!o", "@t", "#t", "$f", "%o", "^o", "&t", "?t", ":d", ";d",
                "qt", "wt", "et", "rt", "tt", "yt", "ut", "it", "ot", "pt",
                "at", "st", "dt", "ft", "gt", "ht", "jt", "kt", "lt", "",
                {k: `${keyboard.svg.shift}t`, n: "shift"}, "zt", "xt", "ct", "vt", "bt", "nt", "mt", "_t", {k: `${keyboard.svg.backspace}t`, n: "backspace"},
                {k: "123n", n: "123", col: "1/3"}, {k: "Spacet", n: " ", col: "3/10", }, {k: "&quot;s", n: "quote"},
            ],
            [
                "!o", "@t", "#t", "$f", "%o", "^o", "&t", "?t", ":d", ";d",
                "Qt", "Wt", "Et", "Rt", "Tt", "Yt", "Ut", "It", "Ot", "Pt",
                "At", "St", "Dt", "Ft", "Gt", "Ht", "Jt", "Kt", "Lt", "",
                {k: `${keyboard.svg.shiftFull}t`, n: "shift"}, "Zt", "Xt", "Ct", "Vt", "Bt", "Nt", "Mt", "_t", {k: `${keyboard.svg.backspaceFull}t`, n: "backspace"},
                {k: "123n", n: "123", col: "1/3"}, {k: "Spacet", n: " ", col: "3/10", }, {k: "'s", n: "quote"},

            ],
        ]],
};

function pressKey(x) {
    //Vibrate if key action actually does something
    if((x === "backspace" || x === "clear" || x === "enter") && textArea.value == "");
    else if(x === "left arrow" && textArea.selectionStart == 0);
    else if(x === "right arrow" && textArea.selectionStart == textArea.value.length);
    else window.navigator.vibrate(15);
    //Focus on text area
    textArea.focus();
    //Get key type
    let key;
    if(typeof x == "string") key = x;
    else key = x.innerText;
    //Set interval if key continues being pressed
    if(keyboard.intervalKey != key) {
        keyboard.clearInterval();
        keyboard.intervalKey = key;
        keyboard.intervalDelay = setTimeout(_ => keyboard.interval = setInterval(_ => pressKey(key), 90), 250);
    }
    //Parse key
    if(key.includes("arrow")) {
        if(key == "left arrow") {
            textArea.setSelectionRange(Math.max(textArea.selectionStart - 1, 0), Math.max(textArea.selectionStart - 1, 0));
        }
        else if(key == "right arrow") {
            textArea.setSelectionRange(textArea.selectionStart + 1, textArea.selectionStart + 1);
        }
    }
    else if(key == "enter") {
        if(keyboard.shift) insertAtCursor(textArea, "\n");
        else textAreaKeydown({key: "Enter", shiftKey: false});
    }
    else if(key == "abc") {
        keyboard.currentPage = "abc";
        keyboard.element.classList.remove("page1");
        keyboard.element.classList.add("page2");
    }
    else if(key == "123") {
        keyboard.currentPage = "123";
        keyboard.element.classList.remove("page2");
        keyboard.element.classList.add("page1");

    }
    else if(key == "clear") {
        textArea.value = "";
        update_highlight(textArea.value);
    }
    else if(key == "backspace") {
        if(textArea.selectionStart == textArea.selectionEnd) {
            let start = textArea.selectionStart;
            textArea.value = textArea.value.substring(0, textArea.selectionStart - 1) + textArea.value.substring(textArea.selectionStart);
            if(textArea.value.length == start - 1) textArea.setSelectionRange(start, start);
            else textArea.setSelectionRange(start - 1, start - 1);
            update_highlight(textArea.value);
        }
        else {
            let [start, finish] = [textArea.selectionStart, textArea.selectionEnd];
            let [begin, end] = [Math.min(start, finish), Math.max(start, finish)];
            textArea.value = textArea.value.substring(0, begin) + textArea.value.substring(end);
            textArea.setSelectionRange(begin, begin);
            update_highlight(textArea.value);
        }
    }
    else if(key == "shift") {
        keyboard.shift = !keyboard.shift;
        if(keyboard.shift) keyboard.element.classList.add("shift");
        else keyboard.element.classList.remove("shift");
    }
    else {
        if(keyboard.shift) {
            keyboard.shift = false;
            keyboard.element.classList.remove("shift");
        }
        if(key == "quote") key = "\"";
        insertAtCursor(textArea, key);
        update_highlight(textArea.value);
    }
}
document.querySelector("#keyboard_open").addEventListener("click", event => {
    keyboard.enabled = !keyboard.enabled;
    //Show/hide keyboard
    if(keyboard.enabled) keyboard.element.classList.add("shown");
    else keyboard.element.classList.remove("shown");
    //Set input mode
    if(keyboard.enabled) textArea.setAttribute("inputmode", "none");
    else textArea.setAttribute("inputmode", "text");
    //Show and hide keyboard close indicator
    if(keyboard.enabled) document.querySelector("#keyboard_open_x").classList.add("show");
    else document.querySelector("#keyboard_open_x").classList.remove("show");
    //Focus textbox
    textArea.focus();
});
window.onmessage = event => {
    let message = event.data;
    if(message.startsWith("openhelppage")) {
        let query = message.substring("openhelppage ".length);
        let data = JSON.parse(Module.query(query));
        if(data.length == 0) {
            console.error("Link for " + query + " did not find result");
            return;
        }
        else displayHelpPage(data[0].id);
    }

};