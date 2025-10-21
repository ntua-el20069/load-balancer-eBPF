from fastapi import FastAPI
import os

app = FastAPI()


@app.get("/", tags=["Root"])  # type: ignore
async def read_root() -> dict:
    num = int(os.getenv("NUM", "0"))
    return {"message": f"welcome from server {num}"}

