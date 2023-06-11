async function logJSONData() {
    const response = await fetch('/api/data');
    const jsonData = await response.json();
    console.log(jsonData);
}

setInterval(logJSONData, 1000)