window.onmessage = function (e) {
    let message = e.data.toString();
    if(message.startsWith("hide")) {
        let pages = document.getElementsByClassName("page_help");
        for(let i = 0; i < pages.length; i++) {
            pages[i].style.display = "none";
        }
    }
    else if(message.startsWith("show")) {
        let page = document.querySelector("#help_" + e.data.substring(4));
        page.style.display = "block";
    }
};
function openHelp(element) {
    window.top.postMessage("openhelppage " + element.innerText);

}