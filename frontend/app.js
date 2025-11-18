const terminal = document.getElementById("terminal");
let currentCommand = ""; 

let currentPath = "C:\\";

const ws = new WebSocket("ws://localhost:8080");

function buildCurrentLineHTML() {
    // DÙNG escapeHTML() cho cả 2 biến
    const safePath = escapeHTML(currentPath);
    const safeCommand = escapeHTML(currentCommand);
    return `<span id="current-line"><span class="prompt">${safePath}></span> ${safeCommand}<span class="cursor"> </span></span>`;
}

function escapeHTML(str) {
    return str.replace(/[&<>"']/g, function(m) {
        return {
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#039;'
        }[m];
    });
}

function appendOutput(text) {
    const currentLine = document.getElementById("current-line");
    if (currentLine) {
        currentLine.remove();
    }
    
    // DÙNG escapeHTML() ở đây!
    terminal.innerHTML += escapeHTML(text) + buildCurrentLineHTML();
    scrollToBottom();
}

function updateCurrentLine() {
    const currentLine = document.getElementById("current-line");
    
    if (currentLine) {
        currentLine.innerHTML = `<span class="prompt">${currentPath}></span> ${currentCommand}<span class="cursor"> </span>`;
    }
    scrollToBottom();
}

function scrollToBottom() {
    window.scrollTo(0, document.body.scrollHeight);
}

ws.onopen = () => {
    console.log("Connected");
    terminal.innerHTML = '--- Da ket noi den server C++ ---\n' + buildCurrentLineHTML();
    terminal.focus(); 
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (data.what === "PATH") {
        currentPath = data.message;
        updateCurrentLine();
        return;
    }
    if (data.what === "Info") {
        if (data.message !== "") {
            appendOutput(data.message + "\n");
        }
        return;
    }
};

ws.onclose = () => {
    appendOutput("--- Da mat ket noi voi server ---\n");
};

document.addEventListener("keydown", (event) => {
    event.preventDefault();
    const key = event.key;

    if (key === "Enter") {        
        const currentLine = document.getElementById("current-line");
        if (currentLine) {
            currentLine.remove();
        }

        terminal.innerHTML += `<span class="prompt">${escapeHTML(currentPath)}></span> ${escapeHTML(currentCommand)}\n`;
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ terminal: currentCommand, path: currentPath }));
        }
        
        currentCommand = "";
        terminal.innerHTML += buildCurrentLineHTML();
        scrollToBottom();

    } else if (key === "Backspace") {
        if (currentCommand.length > 0) {
            currentCommand = currentCommand.substring(0, currentCommand.length - 1);
            updateCurrentLine(); 
        }
    } else if (event.ctrlKey || event.altKey || event.metaKey) {
        // Bỏ qua
    } else if (key.length === 1) {
        currentCommand += key;
        updateCurrentLine(); 
    }
});

terminal.addEventListener("click", () => {
    terminal.focus();
});
window.addEventListener("click", () => {
    terminal.focus();
});