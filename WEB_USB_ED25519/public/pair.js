const pair= ()=>{
    
    let key,device;
    navigator.usb.requestDevice({ filters: [] })
    .then(selectedDevice=>{
        device=selectedDevice;
        return device.open();
    })
    .then(()=>device.selectConfiguration(1))
    .then(()=>device.claimInterface(0))
    .then(()=>{
        const encoder=new TextEncoder();
        // requests the public key
        return device.transferOut(3,encoder.encode('PAIR'));
    })
    // FOR FUTURE ME ensure endpoint numbers are correct everywhere
    .then(()=>device.transferIn(3,32))
    .then(result=>{
        // gets public key
        key=Array.from(new Uint8Array(result.data.buffer));
        const password = prompt('Password: ');
        return fetch('/api/store', {
            method: 'POST',
            body: JSON.stringify({key,password}),
            headers: {'Content-Type': 'application/json'}
    
        });
    })
    .then(res=>{
        if(res.status==401){
            alert('Wrong password');
            throw new Error('Wrong Password');
        }
    })
    .catch(error=>{console.error(error)});
}
export default pair;