#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const String indexHtml = R"=====(
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="iframeUrl" content="https://incubator.katkovonline.com">
<title>Incubator</title>
<style>
    body, html {
        margin: 0;
        padding: 0;
        width: 100%;
        height: 100%;
        overflow: hidden;
    }
    iframe {
        width: 100%;
        height: 100%;
        border: none; /* remove iframe border */
    }
</style>
</head>
<body>

<iframe id="myIframe" src="" frameborder="0"></iframe>

<script>
    // Add an event listener for the window load event
    window.addEventListener('load', function() {
        // Dispatch an event to itself with data 'null' upon loading
        window.dispatchEvent(new CustomEvent('message', { detail: 'null' }));
    });

    window.addEventListener('message', receiveMessage, false);

    function receiveMessage(event) {

        if(!event.data || event.data === 'null') {
            console.log("READ request");
            // Make GET request to retrieve temperature data
            fetch('/api/temp')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Failed to retrieve temperature data');
                }
                return response.json();
            })
            .then(data => {
                console.log("Received data:");
                console.log(data);
                // Parse the received JSON
                const temp = data.temp;
                const setTemp = data.setTemp;
                const state = data.state;

                // Read iframe URL from meta tag
                const iframeUrl = document.querySelector('meta[name="iframeUrl"]').content;

                // Construct the iframe URL with the parsed values
                const fullUrl = `${iframeUrl}/?temp=${temp}&setTemp=${setTemp}&state=${state}`;

                // Set the iframe src attribute
                document.getElementById('myIframe').src = fullUrl;
            })
            .catch(error => {
                console.error('Error:', error);
            });
        }
        else
        {
            console.log('WRITE request');
            // Extract inputVal from the received message
            const inputVal = event.data.inputVal;

            // Make API call
            fetch('/api/temp', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    temperature: inputVal
                })
            })
            .then(response => {
                // Handle the response from the API
                if (response.ok) {
                    console.log('Temperature set successfully');
                } else {
                    console.error('Failed to set temperature');
                }
                // Get reference to the iframe element
                var iframe = document.getElementById('myIframe');

                // Reload the iframe
                // iframe.src = iframe.src;
                window.dispatchEvent(new CustomEvent('message', { detail: 'null' }));
                console.log('refreshed');
            })
            .catch(error => {
                console.error('Error:', error);
            });
        }
    }
</script>

</body>
</html>

)=====";

#endif
