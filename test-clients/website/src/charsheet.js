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
        input: document.getElementById('playerBackgroundInput'),
        pointPoolModifier: 0
    },
    "gender": {
        options: ["Male", 
                  "Female"],
        index: 0,
        element: document.getElementById('selectedPlayerGender'),
        input: document.getElementById('playerGenderInput'),
        pointPoolModifier: 0
    },
};
var playerSkills = {
    "vigour": {
        options: ["1", 
                  "2",
                  "3",
                  "4",
                  "5",
                  "6",
                  "7",
                  "8",
                  "9",
                  "10"],
        index: 0,
        element: document.getElementById('selectedPlayerVigour'),
        input: document.getElementById('playerVigourInput'),
        pointPoolModifier: 1
    },
    "violence": {
        options: ["1", 
                  "2",
                  "3",
                  "4",
                  "5",
                  "6",
                  "7",
                  "8",
                  "9",
                  "10"],
        index: 0,
        element: document.getElementById('selectedPlayerViolence'),
        input: document.getElementById('playerViolenceInput'),
        pointPoolModifier: 1
    },
    "guile": {
        options: ["1", 
                  "2",
                  "3",
                  "4",
                  "5",
                  "6",
                  "7",
                  "8",
                  "9",
                  "10"],
        index: 0,
        element: document.getElementById('selectedPlayerGuile'),
        input: document.getElementById('playerGuileInput'),
        pointPoolModifier: 1
    },
    // Add more player statistics here as needed
};
var pointPool        = 10;
var pointPoolElement = document.getElementById('pointPoolCounter');

function updateSelectedStatOption(statistic) {
    var stat                 = playerStats[statistic];
    stat.element.textContent = stat.options[stat.index];
    stat.input.value         = stat.options[stat.index];
}
function updateSelectedSkillOption(statistic) {
    var stat                 = playerSkills[statistic];
    stat.element.textContent = stat.options[stat.index];
    stat.input.value         = stat.options[stat.index];

    pointPoolElement.textContent = pointPool;
}

function updateStatEventListeners(statistic) {
    var stat = playerStats[statistic];
    
    document.getElementById(statistic + 'LeftArrow').addEventListener('click', function() {
        if (stat.index > 0) {
            stat.index--;
        }
        else {
            stat.index = stat.options.length - 1;
        }
        updateSelectedStatOption(statistic);
    });
    
    document.getElementById(statistic + 'RightArrow').addEventListener('click', function() {
        if (stat.index < stat.options.length - 1) {
            stat.index++;
        }
        else {
            stat.index = 0;
        }
        updateSelectedStatOption(statistic);
    });
}

function updateSkillEventListeners(statistic) {
    var stat = playerSkills[statistic];
    
    document.getElementById(statistic + 'LeftArrow').addEventListener('click', function() {
        if (stat.index > 0) {
            stat.index -= stat.pointPoolModifier;
            pointPool  += stat.pointPoolModifier;
        }
        updateSelectedSkillOption(statistic);
    });
    
    document.getElementById(statistic + 'RightArrow').addEventListener('click', function() {
        if (stat.index < stat.options.length - 1 && pointPool > 0) {
            stat.index += stat.pointPoolModifier;
            pointPool  -= stat.pointPoolModifier;
        }
        updateSelectedSkillOption(statistic);
    });
}

// Initialization....
for (var stat in playerStats) {
    updateStatEventListeners(stat);
}
for (var skill in playerSkills) {
    updateSkillEventListeners(skill);
}
for (var stat in playerStats) {
    updateSelectedStatOption(stat);
}
for (var skill in playerSkills) {
    updateSelectedSkillOption(skill);
}
