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

function resize() {
    let ioWidth = document.querySelector("#io").clientWidth;
    let width = window.innerWidth;
    let height = window.innerHeight;
    if(ioWidth < 700) {
        document.querySelector("#input").style.width = "100%";
        document.querySelector("#output").style.width = "100%";
    }
    else {
        document.querySelector("#input").style.width = "50%";
        document.querySelector("#output").style.width = "50%";
    }
}