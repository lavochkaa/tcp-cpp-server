(function () {
    const messagesBox = document.getElementById("messages");
    const chatUrl = document.body.dataset.chatUrl;

    if (!messagesBox || !chatUrl) {
        return;
    }

    let lastHtml = "";

    async function refreshMessages() {
        try {
            const response = await fetch(chatUrl, { cache: "no-store" });
            if (!response.ok) {
                return;
            }

            const html = await response.text();
            if (html !== lastHtml) {
                messagesBox.innerHTML = html;
                messagesBox.scrollTop = messagesBox.scrollHeight;
                lastHtml = html;
            }
        } catch (error) {
        }
    }

    refreshMessages();
    setInterval(refreshMessages, 1000);
})();
