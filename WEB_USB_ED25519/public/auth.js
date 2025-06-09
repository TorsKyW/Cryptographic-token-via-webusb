const secret=document.querySelector('.secret');
const start = document.querySelector('.start');
const hide=()=>{
    secret.innerHTML = '';
    secret.style.display='none';
    start.style.display='flex';
}
const authorize=(signature,message,device)=>{
    let Signature=signature,Message=message;
    // sends a signature verification request to the server
    fetch('/private', {
        method: 'POST',
        body:JSON.stringify({Signature,Message}),
        headers: {'Content-Type': 'application/json'}
    })
    .then(res=>{
        if (res.ok) {
            // returns the secret content
            return res.text();
        }else{
            // disables secret access (just in case) and exits the chain
            hide();
            throw new Error('Unauthorized');
        }
    })
    .then(html=>{
        // hides the starting page and shows the secret content
        secret.innerHTML = html;
        secret.style.display='flex';
        start.style.display='none';
        // disables the secret access upon device disconnection
        navigator.usb.addEventListener('disconnect',(e)=>{
            if(e.device===device){hide();}
        });
    })
    .catch(err=>{
        console.error(err);
    });
}
export default authorize;
