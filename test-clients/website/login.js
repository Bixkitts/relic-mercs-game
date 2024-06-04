document.getElementById("loginForm").addEventListener("submit", function(event) {
  event.preventDefault(); 

  var formData       = new FormData(this);
  var urlEncodedData = new URLSearchParams(formData).toString();

  fetch("/login", {
    method: "POST",
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    body: urlEncodedData
  })
  .then(response => {
    if (response.ok) {
       return response.text();
    } if (response.status === 400) {
       document.getElementById("credWarning").innerText = "Incorrect game or player password. Please try again.";
       throw new Error("Bad credentials!");
    } else {
       throw new Error("Bad response!"); 
    }
  })
  .then(html => {
    const parser = new DOMParser();
    const newDoc = parser.parseFromString(html, 'text/html');
    
    // Replace the current document with the new one
    document.open();
    document.write(newDoc.documentElement.outerHTML);
    document.close();
  })
  .catch(error => {
    console.error("Error:", error);
  });
});

document.getElementById("loginForm").addEventListener("input", function(event) {
  document.getElementById("credWarning").innerText = " "; // Clear warning message
});

document.getElementById('skip').addEventListener('click', function() {
   document.getElementById("playerName").value     = "Test";
   document.getElementById("playerPassword").value = "Test";
   document.getElementById("gamePassword").value   = "hello";
});
