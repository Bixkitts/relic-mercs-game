var options = ["Knight", "Paladin", "Salad", "Cleric"];
var currentIndex = 0;
var selectedOptionElement = document.getElementById('selectedPlayerClass');
var selectedOptionInput = document.getElementById('playerClassInput');

function updateSelectedOption() {
    selectedOptionElement.textContent = options[currentIndex];
    selectedOptionInput.value = options[currentIndex];
}

document.getElementById('leftArrow').addEventListener('click', function() {
    if (currentIndex > 0) {
        currentIndex--;
        updateSelectedOption();
    }
});

document.getElementById('rightArrow').addEventListener('click', function() {
    if (currentIndex < options.length - 1) {
        currentIndex++;
        updateSelectedOption();
    }
});

// Initialize the selected option
updateSelectedOption();
