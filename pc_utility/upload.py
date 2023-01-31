from firebase_admin import credentials, initialize_app, storage

cred = credentials.Certificate("max78000-img-capture-firebase-adminsdk-3qmu4-5b275f2270.json")
initialize_app(cred, {'storageBucket': 'max78000-img-capture.appspot.com'})

def upload_to_firebase(fileName):
    # Put your local file path
    bucket = storage.bucket()
    blob = bucket.blob(fileName)
    blob.upload_from_filename(fileName)

    # Opt : if you want to make public access from the URL
    blob.make_public()

    print("your file url", blob.public_url)

