import authorize from './auth.js';
const connect=()=>{
    let device,message='',signature1,signature2;
    navigator.usb.requestDevice({ filters: [{vendorId:0xCafe}] })
    .then(dev=>{
        device=dev;
        for(let i=0;i<13;i++){
            // generates a random message to prevent duplicate signatures
            message+=String.fromCharCode(Math.floor(Math.random() * (127 - 33) + 33));
        }
        return device.open();
    })
    .then(()=> device.selectConfiguration(1))
    .then(()=> device.claimInterface(2))
    .then(()=>{
        // sends a request with the message and requests a signature
        const encoder=new TextEncoder();
        return device.transferOut(3,encoder.encode(message));
    })
    .then(() => device.transferIn(3, 32))
    .then(result=>{
        signature1 = new Uint8Array(result.data.buffer);
        return device.transferIn(3,32);
    }).then(result=>{
        signature2 = new Uint8Array(result.data.buffer);
        signature1=Array.from(signature1);
        signature2=Array.from(signature2);
        const fullSignature=signature1.concat(signature2);
        authorize(fullSignature,message,device);

    }).catch(error=>{console.error(error)});
}
export default connect;
