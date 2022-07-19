function evaluate(xpr) {
    try {
        return Module.evaluate(xpr);
    }
    catch(e) {
        console.error(e);
    }

}