function evaluate(xpr) {
    try {
        return Module.evaluate(xpr);
    }
    catch(e) {
        console.error(e);
    }
}
function run_line(str, coloredInput = "") {
    if(coloredInput == "") coloredInput = Module.highlightExpression(str);
    let out = document.createElement("div");
    out.className = "output_element";
    out.innerHTML = coloredInput + "<br>" + Module.runLineWithColor(str);
    document.querySelector("#input_element").value = "";
    document.querySelector("#highlight_output").innerHTML = "";
    document.querySelector("#output").prepend(out);
}

function update_highlight(str) {
    this.textArea = document.querySelector("#input_element");
    this.output = document.querySelector("#highlight_output");
    if(str[str.length - 1] == '\n') {
        if(str.length == 1)
            this.textArea.value = "";
        else run_line(str.substring(0, str.length - 1), this.output.innerHTML);
        return;
    }
    this.text = str;
    this.textArea.style.height = "1px";
    document.querySelector("#input").style.height = (this.textArea.scrollHeight) + "px";
    this.textArea.style.height = (this.textArea.scrollHeight) + "px";
    this.output.innerHTML = Module.highlightLine(this.text);
}
window.onload = _ => {
    resize();
    window.addEventListener("resize", resize);
    update_highlight("");
}

var panelData = {
    isOpen: false,
    page: "help"
};

function panelPage(open) {
    if(open == false)
        panelData.isOpen = false;
    else {
        panelData.isOpen = true;
        if(panelData.page) document.querySelector("#header_button_" + panelData.page).classList.remove("header_button_active");
        if(panelData.page) document.querySelector("#" + panelData.page + "_panel").classList.remove("panel_page_active");
        panelData.page = open;
        document.querySelector("#header_button_" + panelData.page).classList.add("header_button_active");
        document.querySelector("#" + panelData.page + "_panel").classList.add("panel_page_active");
    }
    resize();
    setTimeout(resize, 500 / 3);
    setTimeout(resize, 500 * 2 / 3);
    setTimeout(resize, 500);
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
}
function helpSearch(event, queryBox) {
    if(event.key == "Enter") {
        document.querySelector("#help_page").classList.remove("shown");
        document.querySelector("#search_results").style.display = "block";
        let data = JSON.parse(Module.query(queryBox.value.toString()));
        let queryResults = "";
        for(let i = 0; i < data.length; i++) {
            queryResults += "<div class='search_result' onclick='displayHelpPage(" + data[i].id + ")'>" + data[i].name + "</div>";
        }
        if(data.length == 0)
            queryResults += "No results found for " + queryBox.value;
        document.querySelector("#search_results").innerHTML = queryResults;
        queryBox.value = "";
    }
}
function displayHelpPage(id) {
    document.querySelector("#search_results").style.display = "none";
    document.querySelector("#help_page").classList.add("shown");
    document.querySelector("#help_page").contentWindow.postMessage("hide");
    document.querySelector("#help_page").contentWindow.postMessage("show" + id);
    setTimeout(_ => {
        document.querySelector("#help_page").style.height = "1px";
        document.querySelector("#help_page").style.height = document.querySelector("#help_page").contentWindow.document.body.scrollHeight + 20 + "px";
    }, 0);
}
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