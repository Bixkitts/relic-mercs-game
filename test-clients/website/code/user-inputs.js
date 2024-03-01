let mov      = [0.0, 0.0, 0.0, 0.0];

const speed  = 0.05;

function initWASD(canvas)
{
    document.addEventListener('keydown', function(event) {
        switch(event.key) {
            case 'w':
            case 'W':
                mov[0] = speed;
                break;
            case 's':
            case 'S':
                mov[2] = speed;
                break;
            // Add cases for 'a' and 'd' keys if needed
        }
    });
    document.addEventListener('keyup', function(event) {
        switch(event.key) {
            case 'w':
            case 'W':
                mov[0] = 0;
                break;
            case 's':
            case 'S':
                mov[2] = 0;
                break;
            // Add cases for 'a' and 'd' keys if needed
        }
    });
    document.addEventListener('keydown', function(event) {
        switch(event.key) {
            case 'd':
            case 'D':
                mov[3] = speed;
                break;
            case 'a':
            case 'A':
                mov[1] = speed;
                break;
            // Add cases for 'a' and 'd' keys if needed
        }
    });
    document.addEventListener('keyup', function(event) {
        switch(event.key) {
            case 'd':
            case 'D':
                mov[3] = 0;
                break;
            case 'a':
            case 'A':
                mov[1] = 0;
                break;
            // Add cases for 'a' and 'd' keys if needed
        }
    });
}

function getKeyPan(index)
{
    return mov[index];
}

export {initWASD};
export {getKeyPan};
