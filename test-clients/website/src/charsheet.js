var playerStats = {
    "background": {
        options: ["Trader", 
                  "Farmer", 
                  "Warrior", 
                  "Priest", 
                  "Cultist",
                  "Diplomat",
                  "Slaver",
                  "Monster Hunter",
                  "Clown"],
        index: 0,
        element: document.getElementById('selectedPlayerBackground'),
        input: document.getElementById('playerBackgroundInput')
    },
    "gender": {
        options: ["Male", 
                  "Female"],
        index: 0,
        element: document.getElementById('selectedPlayerGender'),
        input: document.getElementById('playerGenderInput')
    },
    // Add more player statistics here as needed
};

function updateSelectedOption(statistic) {
    var stat                 = playerStats[statistic];
    stat.element.textContent = stat.options[stat.index];
    stat.input.value         = stat.options[stat.index];
}

// Update event listeners for left arrow and right arrow
function updateEventListeners(statistic) {
    var stat = playerStats[statistic];
    
    document.getElementById(statistic + 'LeftArrow').addEventListener('click', function() {
        if (stat.index > 0) {
            stat.index--;
        }
        else {
            stat.index = stat.options.length - 1;
        }
        updateSelectedOption(statistic);
    });
    
    document.getElementById(statistic + 'RightArrow').addEventListener('click', function() {
        if (stat.index < stat.options.length - 1) {
            stat.index++;
        }
        else {
            stat.index = 0;
        }
        updateSelectedOption(statistic);
    });
}

// Initialize event listeners for each statistic
for (var stat in playerStats) {
    updateEventListeners(stat);
}
