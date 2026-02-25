document.addEventListener("DOMContentLoaded", () => {
    const url = "https://github.com/yourname/tgen";

    const btn = document.createElement("a");
    btn.href = url;
    btn.target = "_blank";
    btn.className = "github-corner";
    btn.setAttribute("aria-label", "View source on GitHub");

    btn.innerHTML = `
    <svg viewBox="0 0 250 250" width="60" height="60" aria-hidden="true">
        <path d="M0,0 L115,115 L130,115 L142,142 L250,250 L250,0 Z"></path>
        <path class="octo-arm"
            d="M128,109 C113,99 119,89 119,89 C122,82 120,78 120,78
               C119,72 123,76 123,76 C127,80 125,87 125,87
               C122,97 130,101 134,103"
            fill="currentColor"></path>
        <path class="octo-body"
            d="M115,115 C114,115 115,115 115,115 C115,115 115,115 115,115
               C115,115 115,115 115,115"
            fill="currentColor"></path>
    </svg>`;

    document.body.appendChild(btn);
});
