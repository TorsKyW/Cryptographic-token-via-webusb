import authorize from "./auth.js";
const test=()=>{
    let Message='';
    const device=10;
    for(let i=0;i<13;i++){
        // generates a random message to prevent duplicate signatures
        Message+=String.fromCharCode(Math.floor(Math.random() * (127 - 33) + 33));
    }
    // the api requests a signature for the message as well as a public key that needs to be stored in the database
    fetch('/api/test',{
        method: 'POST',
        body:JSON.stringify({Message}),
        headers: {'Content-Type': 'application/json'}
    }).then(res=>{
        return res.json();
    }).then(data=>{
        // stores the public key and tests the funcionality of the authorize function
        const Signature=data.signature;
        const key=data.publicKeyArray;
        fetch('/api/store',{
            method: 'POST',
            body:JSON.stringify({key,password:'1234'}),
            headers: {'Content-Type': 'application/json'}
        }).then(res=>{
            if(res.ok){
                authorize(Signature,Message,device);
            }else{
                throw new Error('response Failed');
            }
        })
    }).catch(e=>{
        console.error(e);
    });
}
export default test;