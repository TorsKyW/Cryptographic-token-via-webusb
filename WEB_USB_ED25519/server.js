const express = require('express');
const https = require('https');
const fs = require('fs');
const path = require('path');
const nacl = require('tweetnacl');
nacl.util = require('tweetnacl-util');

const app = express();
app.use(express.json());
app.use(express.static('public'));
const DATA_FILE = 'data.json';
let data = {};
if (fs.existsSync(DATA_FILE)){
    data=JSON.parse(fs.readFileSync(DATA_FILE,'utf-8'));
}
// checks if the pairing password is correct
const checkPasswd =(req,res,next)=>{
    if(req.body.password=='1234'){
        next();
    }else{
        res.status(401).send("Unauthorized");
    }
}
// adds a public key to the database
app.post('/api/store',checkPasswd, (req,res)=>{
    const {key}=req.body;
    if(!key){
        return res.status(400).send('Key required');
    }
    const KeyBase64=nacl.util.encodeBase64(Uint8Array.from(key));
    data[KeyBase64]='Public Key';
    fs.writeFileSync(DATA_FILE, JSON.stringify(data,null,2));
    res.status(200).send('Data stored');
});
// sends the necessary variables for testing the vsignature verification
// app.post('/api/test',(req,res)=>{
//     const Message=req.body.Message;
//     const messageUint8= nacl.util.decodeUTF8(req.body.Message);
//     const KeyPair = nacl.sign.keyPair();
//     const publicKeyArray = Array.from(KeyPair.publicKey);
//     const signature = Array.from(nacl.sign.detached(messageUint8,KeyPair.secretKey));
//     res.status(200).json({publicKeyArray,signature,Message});
// });


// verification of signature
const isAuthorized=(req,res,next)=>{
    const message=nacl.util.decodeUTF8(req.body.Message);
    const signature=Uint8Array.from(req.body.Signature);
    for(let key in data){
        if(nacl.sign.detached.verify(message,signature,nacl.util.decodeBase64(key))){
            return next();
        }
    }
    res.status(401).send('Unauthorized')
}
// sends the secret content to the client
app.post('/private', isAuthorized,(req,res)=>{
    res.status(200).sendFile(path.join(__dirname, 'secret.html'));
})

const options = {
    key: fs.readFileSync('key.pem'),
    cert: fs.readFileSync('cert.pem')
};

https.createServer(options, app).listen(443, () => {
    console.log('Server running at https://localhost');
});