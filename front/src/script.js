let ws;
let reconnectInterval = 5000;
let currentMood = "waiting";

// Buddy moods with emojis and messages
const moods = {
    empty: {
        emoji: "😴",
        status: "Esperando por você...",
        message: "Venha sente-se um pouco! Sinto sua falta!",
        className: "worried"
    },
    good: {
        emoji: "😄",
        status: "Postura Perfeita!",
        message: "Você está indo muito bem! Continue assim! Suas costas vão te agradecer depois!",
        className: "happy",
        healthColor: "from-green-500 to-green-400"
    },
    warning: {
        emoji: "😟",
        status: "Hum... Não está bem certo.",
        message: "Estou ficando um pouco preocupado... Você pode ajustar um pouco a sua postura?",
        className: "worried",
        healthColor: "from-yellow-500 to-yellow-400"
    },
    bad: {
        emoji: "😢",
        status: "Oh não! Postura toda torta!",
        message: "Por favor, sente-se direto! Estou realmente preocupado com suas costas!",
        className: "sad",
        healthColor: "from-red-500 to-red-400"
    }
};

function connectWebSocket() {
    // Connect to WebSocket server
    ws = new WebSocket(`ws://${window.location.hostname}:3000`);

    ws.onopen = () => {
        console.log("Connected to WebSocket server");
        const statusEl = document.getElementById("connectionStatus");
        statusEl.classList.remove("bg-gray-300");
        statusEl.classList.add("bg-green-500", "shadow-lg", "shadow-green-500/50");
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            console.log("Received:", data);
            updateDashboard(data);
        } catch (e) {
            console.error("Error parsing message:", e);
        }
    };

    ws.onerror = (error) => {
        console.error("WebSocket error:", error);
    };

    ws.onclose = () => {
        console.log("WebSocket connection closed");
        const statusEl = document.getElementById("connectionStatus");
        statusEl.classList.remove("bg-green-500", "shadow-lg", "shadow-green-500/50");
        statusEl.classList.add("bg-gray-300");

        // Attempt to reconnect
        setTimeout(connectWebSocket, reconnectInterval);
    };
}

function updateDashboard(data) {
    const mood = moods[data.postureStatus] || moods.empty;
    const buddy = document.getElementById("buddy");
    const statusText = document.getElementById("statusText");
    const moodMessage = document.getElementById("moodMessage");
    const healthBar = document.getElementById("healthBar");
    const goodTime = document.getElementById("goodTime");
    const badTime = document.getElementById("badTime");
    const sessionTime = document.getElementById("sessionTime");

    // Update buddy
    if (currentMood !== data.postureStatus) {
        buddy.className = "buddy text-8xl inline-block transition-all duration-500 " + mood.className;
        buddy.textContent = mood.emoji;

        // Add particles for good posture
        if (data.postureStatus === "good") {
            createParticles();
        }

        currentMood = data.postureStatus;
    }

    // Update text
    statusText.textContent = mood.status;
    moodMessage.textContent = mood.message;

    // Update health bar
    const score = data.postureScore || 100;
    healthBar.style.width = score + "%";
    healthBar.textContent = score + "%";

    // Update health bar color
    healthBar.className = "health-fill h-full bg-gradient-to-r flex items-center justify-center text-white font-bold text-sm " + mood.healthColor;

    // Update stats
    goodTime.textContent = formatTime(data.goodTime);
    badTime.textContent = formatTime(data.badTime);
    sessionTime.textContent = "Session: " + formatTime(data.sessionTime);
}

function formatTime(seconds) {
    if (seconds < 60) return seconds + "s";
    const minutes = Math.floor(seconds / 60);
    return minutes + "m";
}

function createParticles() {
    const container = document.getElementById("particles");
    const emojis = ["✨", "⭐", "💫", "🌟"];

    for (let i = 0; i < 5; i++) {
        setTimeout(() => {
            const particle = document.createElement("div");
            particle.className = "particle absolute text-2xl";
            particle.textContent = emojis[Math.floor(Math.random() * emojis.length)];
            particle.style.left = Math.random() * 100 + "%";
            particle.style.top = "50%";
            container.appendChild(particle);

            setTimeout(() => particle.remove(), 2000);
        }, i * 100);
    }
}

// Connect on load
connectWebSocket();