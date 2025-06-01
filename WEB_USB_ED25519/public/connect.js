import authorize from './auth.js';
const connect=()=>{
    let device,message='',signature;
    navigator.usb.requestDevice({ filters: [] })
    .then(dev=>{
        device=dev;
        for(let i=0;i<13;i++){
            // generates a random message to prevent duplicate signatures
            message+=String.fromCharCode(Math.floor(Math.random() * (127 - 33) + 33));
        }
        return device.open();
    })
    .then(()=> device.selectConfiguration(1))
    .then(()=> device.claimInterface(0))
    .then(()=>{
        // sends a request with the message and requests a signature
        const encoder=new TextEncoder();
        return device.transferOut(3,encoder.encode(message));
    })
    .then(() => device.transferIn(3, 64))
    .then(result=>{
        signature = new Uint8Array(result.data.buffer);
        authorize(signature,message,device);
    }).catch(error=>{console.error(error)});
}
export default connect;