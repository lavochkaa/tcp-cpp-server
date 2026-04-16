(function () {
    const messagesBox = document.getElementById("messages");
    const chatUrl = document.body.dataset.chatUrl;

    if (!messagesBox || !chatUrl) {
        return;
    }

    let lastHtml = "";

    function isNearBottom() {
        const rest = messagesBox.scrollHeight - messagesBox.scrollTop - messagesBox.clientHeight;
        return rest < 60;
    }

    async function refreshMessages() {
        try {
            const keepPinnedToBottom = isNearBottom();
            const response = await fetch(chatUrl, { cache: "no-store" });
            if (!response.ok) {
                return;
            }

            const html = await response.text();
            if (html !== lastHtml) {
                messagesBox.innerHTML = html;

                if (keepPinnedToBottom || !lastHtml) {
                    messagesBox.scrollTop = messagesBox.scrollHeight;
                }

                lastHtml = html;
            }
        } catch (error) {
        }
    }

    refreshMessages();
    setInterval(refreshMessages, 1000);
})();
