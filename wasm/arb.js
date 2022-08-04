//This function is only called after the first instance of an arb class is needed

var arbIsLoaded = false;
var arb;
var mp;
var exp_ptr;
function loadArb() {
    if(!arbIsLoaded) {
        let script = document.createElement("script");
        script.setAttribute("src", "https://cdn.jsdelivr.net/npm/gmp-wasm@1.1.0/dist/index.umd.min.js");
        script.setAttribute("async", "true");
        document.body.appendChild(script);
        script.addEventListener("load", _ => {
            console.log("Arb script loaded");
            gmp.init().then(g => {
                console.log("Arb is loaded successfully");
                arb = g.binding;
                arbIsLoaded = true;
                exp_ptr = arb.malloc(4);
            });
        }, false);
    }
    return;
}

const arbNumbers = [];
var round = 0;//MPFR_RNDN (round nearest)

function runArb(name, arg1, arg2 = -1) {
    let outPtr;
    try {
        if(arg2 == -1) {
            outPtr = mallocArb(arb.mpfr_get_prec(arg1), true);
            arb[name](outPtr, arg1, round);
        }
        else {
            outPtr = mallocArb(Math.max(arb.mpfr_get_prec(arg1), arb.mpfr_get_prec(arg2)), true);
            if((err = arb[name](outPtr, arg1, arg2, round))) console.log("error in operation: " + err);
        }
    } catch(e) {console.error(e);}
    return outPtr;
}
function arbCompare(name, arg1, arg2) {
    let out = arb[name](arg1, arg2);
    if(out == 0) return 0;
    else return 1;
}
function arbProperty(name, arg1) {
    return arb[name](arg1);
}
function arbConstant(name, prec) {
    const outPtr = mallocArb(prec, false)
    arb[name](outPtr, round);
    return outPtr;
}
function freeArb(ptr) {
    arb.free(ptr);
}
function mallocArb(prec, isBinary = false) {
    const out = arb.mpfr_t();
    if(isBinary) arb.mpfr_init2(out, prec);
    else arb.mpfr_init2(out, gmp.precisionToBits(prec));
    return out;
}
function arbToString(ptr, base = 10) {
    //Get string length
    let n = 1 + Math.ceil(arb.mpfr_get_prec(ptr) * Math.log(2) / Math.log(base));
    //Print string
    let strPtr = arb.mpfr_get_str(0, exp_ptr, base, n, ptr, round);
    //Parse exponenet and string from mpfr memory
    let exp = arb.memView.getInt32(exp_ptr, true);
    let str = new TextDecoder().decode(arb.memView.buffer.slice(strPtr, strPtr + n));
    //Add period to string
    if(str[0] == '-') str = str.substring(0, 2) + "." + str.substring(2);
    else str = str.substring(0, 1) + "." + str.substring(1);
    //Return string with or without exponent suffix
    if(exp == 1) return str;
    return str + "@" + (exp > 0 ? "+" : "") + (exp - 1);
}
function bindArbToString(ptr, base) {
    return allocateUTF8OnStack(arbToString(ptr, base));
}
function stringToArb(str, prec, base = 10) {
    const outPtr = mallocArb(prec, false);
    const strPtr = arb.malloc_cstr(str);
    arb.mpfr_set_str(outPtr, strPtr, base, round);
    return outPtr;
}
function arbBindStringToArb(strPtr, prec, base = 10) {
    return stringToArb(UTF8ToString(strPtr), prec, base);
}
function arbToDouble(ptr) {
    return arb.mpfr_get_d(ptr, round);
}
function doubleToArb(dub, prec) {
    const outPtr = mallocArb(prec, false);
    arb.mpfr_set_d(outPtr, dub, round);
    return outPtr;
}
function arbSetExp(ptr, exp) {
    arb.mpfr_set_exp(ptr, exp);
}