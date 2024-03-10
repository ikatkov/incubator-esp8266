window.addEventListener('message', receiveMessageFromParent);

function receiveMessageFromParent(event) {
    // Ensure the message is coming from the expected source
    // In this case, the parent window
    if (event.source !== parent) {
        console.error('Unexpected source:', event.source);
        return;
    }
    console.log("Received event");
    console.log(event);
    if (event.data.messageType != undefined && event.data.messageType == "ApiPostACK") {
        // Check if the message contains the data we're expecting
        NProgress.done();
    } else if (event.data.messageType != undefined && event.data.messageType == "ApiGet") {
        // Check if the message contains the data we're expecting
        if (event.data.temp != undefined && event.data.setTemp != undefined && event.data.state != undefined) {
            // Process the received data
            const temp = event.data.temp;
            const setTemp = event.data.setTemp;
            const state = event.data.state;


            thermostat.setValues(temp, setTemp, state);
        }
    }
}

class NeuThermostat {
    constructor(el, temp = 0, setTemp = 0, state = "Idle") {
        this.el = document.querySelector(el);
        this.temp = temp;
        this.tempMin = 10;
        this.tempMax = 61;
        this.angleMin = 15;
        this.angleMax = 345;
        this.setTemp = setTemp;
        this.state = state;
        this.init();
    }
    init() {
        window.addEventListener("keydown", this.kbdEvent.bind(this));
        window.addEventListener("keyup", this.activeState.bind(this));

        // hard limits
        if (this.tempMin < 0)
            this.tempMin = 0;

        if (this.tempMax > 99)
            this.tempMax = 99;

        if (this.angleMin < 0)
            this.angleMin = 0;

        if (this.angleMax > 360)
            this.angleMax = 360;

        // init values
        this.tempSet(this.temp);
        this.stateAdjust(this.setTemp, this.state);

        // init GreenSock Draggable
        Draggable.create(".temp__drag", {
            type: "rotation",
            bounds: {
                minRotation: this.angleMin,
                maxRotation: this.angleMax
            },
            onDrag: () => {
                this.setTempAdjust("drag");
                let mainHeading = document.querySelector(".main__heading");
                mainHeading.textContent = "Set Temperature";
            },
            onDragEnd: () => {
                this.apiSetTemperature(Math.round(this.setTemp));
                let mainHeading = document.querySelector(".main__heading");
                mainHeading.textContent = "Current Temperature";
            }
        });
    }

    setValues(temp, setTemp, state) {
        this.state = state;
        this.setTemp = setTemp;

        this.tempSet(temp);
        this.stateAdjust(this.setTemp, this.state);
    }

    angleFromMatrix(transVal) {
        let matrixVal = transVal.split('(')[1].split(')')[0].split(','),
            [cos1, sin] = matrixVal.slice(0, 2),
            angle = Math.round(Math.atan2(sin, cos1) * (180 / Math.PI)) * -1;

        // convert negative angles to positive
        if (angle < 0)
            angle += 360;

        if (angle > 0)
            angle = 360 - angle;

        return angle;
    }

    kbdEvent(e) {
        let kc = e.keyCode;

        if (kc) {
            // up or right
            if (kc == 38 || kc == 39)
                this.setTempAdjust("u");

            // left or down
            else if (kc == 37 || kc == 40)
                this.setTempAdjust("d");
        }
    }
    activeState(shouldAdd = false) {
        if (this.el) {
            let dragClass = "temp__drag",
                activeState = `${dragClass}--active`,
                tempDrag = this.el.querySelector(`.${dragClass}`);

            if (tempDrag) {
                if (shouldAdd === true)
                    tempDrag.classList.add(activeState);
                else
                    tempDrag.classList.remove(activeState);
            }
        }
    }
    removeClass(el, classname) {
        el.classList.remove(classname);
    }
    changeDigit(el, digit) {
        el.textContent = digit;
    }

    apiSetTemperature(inputVal) {
        console.log("setTemperature", inputVal);
        NProgress.start();
        window.parent.postMessage({
            inputVal
        }, '*');
    }

    setTempAdjust(inputVal) {
        /*
        inputVal can be the temp as an integer, "u" for up, 
        "d" for down, or "drag" for dragged value
        */
        if (this.el) {
            let cs = window.getComputedStyle(this.el),
                tempDigitEls = this.el.querySelectorAll(".temp__digit"),
                tempDigits = tempDigitEls ? Array.from(tempDigitEls).reverse() : [],
                tempDrag = this.el.querySelector(".temp__drag"),
                cold = this.el.querySelector(".temp__shade-cold"),
                hot = this.el.querySelector(".temp__shade-hot"),
                prevTemp = Math.round(this.setTemp),
                tempRange = this.tempMax - this.tempMin,
                angleRange = this.angleMax - this.angleMin,
                notDragged = inputVal != "drag";

            // input is an integer
            if (!isNaN(inputVal)) {
                this.setTemp = inputVal;

                // input is a given direction
            } else if (inputVal == "u") {
                if (this.setTemp < this.tempMax)
                    ++this.setTemp;

                this.activeState(true);

            } else if (inputVal == "d") {
                if (this.setTemp > this.tempMin)
                    --this.setTemp;

                this.activeState(true);

                // Draggable was used
            } else if (inputVal == "drag") {
                if (tempDrag) {
                    let tempDragCS = window.getComputedStyle(tempDrag),
                        trans = tempDragCS.getPropertyValue("transform"),
                        dragAngle = this.angleFromMatrix(trans),
                        relAngle = dragAngle - this.angleMin,
                        angleFrac = relAngle / angleRange;

                    this.setTemp = angleFrac * tempRange + this.tempMin;
                }
            }

            // keep the temperature within bounds
            if (this.setTemp < this.tempMin)
                this.setTemp = this.tempMin;
            else if (this.setTemp > this.tempMax)
                this.setTemp = this.tempMax;

            // use whole number temperatures for keyboard control
            this.setTemp = Math.round(this.setTemp);

            let relTemp = this.setTemp - this.tempMin,
                tempFrac = relTemp / tempRange,
                angle = tempFrac * angleRange + this.angleMin;

            // CSS variable
            this.el.style.setProperty("--angle", `${angle}deg`);

            // draggable area
            if (tempDrag && notDragged)
                tempDrag.style.transform = `rotate(${angle}deg)`;

            // shades
            if (cold)
                cold.style.opacity = 1 - tempFrac;
            if (hot)
                hot.style.opacity = tempFrac;

            // display value
            if (tempDigits) {
                let prevDigitArr = String(prevTemp).split("").reverse(),
                    tempRounded = Math.round(this.setTemp),
                    digitArr = String(tempRounded).split("").reverse(),
                    maxDigits = 2,
                    digitDiff = maxDigits - digitArr.length,
                    prevDigitDiff = maxDigits - prevDigitArr.length,
                    incClass = "temp__digit--inc",
                    decClass = "temp__digit--dec",
                    timeoutA = 150,
                    timeoutB = 300;

                while (digitDiff--)
                    digitArr.push("");

                while (prevDigitDiff--)
                    prevDigitArr.push("");

                for (let d = 0; d < maxDigits; ++d) {
                    let digit = +digitArr[d],
                        prevDigit = +prevDigitArr[d],
                        tempDigit = tempDigits[d];

                    setTimeout(this.changeDigit.bind(null, tempDigit, digit), timeoutA);

                    // animate increment
                    if ((digit === 0 && prevDigit === 9) || (digit > prevDigit && this.setTemp > prevTemp)) {
                        this.removeClass(tempDigit, incClass);
                        void tempDigit.offsetWidth;
                        tempDigit.classList.add(incClass);
                        setTimeout(this.removeClass.bind(null, tempDigit, incClass), timeoutB);

                        // animate decrement
                    } else if ((digit === 9 && prevDigit === 0) || (digit < prevDigit && this.setTemp < prevTemp)) {
                        this.removeClass(tempDigit, decClass);
                        void tempDigit.offsetWidth;
                        tempDigit.classList.add(decClass);
                        setTimeout(this.removeClass.bind(null, tempDigit, decClass), timeoutB);
                    }
                }
            }
        }
    }

    tempSet(inputVal) {
        if (this.el) {
            let cs = window.getComputedStyle(this.el),
                tempDigitEls = this.el.querySelectorAll(".temp__digit"),
                tempDigits = tempDigitEls ? Array.from(tempDigitEls).reverse() : [],
                prevTemp = Math.round(this.temp)

            this.temp = inputVal;

            // display value
            if (tempDigits) {
                let prevDigitArr = String(prevTemp).split("").reverse(),
                    tempRounded = Math.round(this.temp),
                    digitArr = String(tempRounded).split("").reverse(),
                    maxDigits = 2,
                    digitDiff = maxDigits - digitArr.length,
                    prevDigitDiff = maxDigits - prevDigitArr.length,
                    incClass = "temp__digit--inc",
                    decClass = "temp__digit--dec",
                    timeoutA = 150,
                    timeoutB = 300;

                while (digitDiff--)
                    digitArr.push("");

                while (prevDigitDiff--)
                    prevDigitArr.push("");

                for (let d = 0; d < maxDigits; ++d) {
                    let digit = +digitArr[d],
                        prevDigit = +prevDigitArr[d],
                        tempDigit = tempDigits[d];

                    setTimeout(this.changeDigit.bind(null, tempDigit, digit), timeoutA);

                    // animate increment
                    if ((digit === 0 && prevDigit === 9) || (digit > prevDigit && this.temp > prevTemp)) {
                        this.removeClass(tempDigit, incClass);
                        void tempDigit.offsetWidth;
                        tempDigit.classList.add(incClass);
                        setTimeout(this.removeClass.bind(null, tempDigit, incClass), timeoutB);

                        // animate decrement
                    } else if ((digit === 9 && prevDigit === 0) || (digit < prevDigit && this.temp < prevTemp)) {
                        this.removeClass(tempDigit, decClass);
                        void tempDigit.offsetWidth;
                        tempDigit.classList.add(decClass);
                        setTimeout(this.removeClass.bind(null, tempDigit, decClass), timeoutB);
                    }
                }
            }
        }
    }

    stateAdjust(setTemp, state) {
        let outdoorEls = this.el.querySelectorAll(".temp__o-value"),
            outdoorVals = outdoorEls ? Array.from(outdoorEls) : [];

        this.setTemp = setTemp;
        this.state = state;

        if (outdoorVals) {
            outdoorVals[0].textContent = `${this.setTemp}°`;
            outdoorVals[1].textContent = `${this.state}`;
        }
    }
}

const thermostat = new NeuThermostat(".temp");