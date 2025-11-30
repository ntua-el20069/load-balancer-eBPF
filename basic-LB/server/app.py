from fastapi import FastAPI, HTTPException
import os

app = FastAPI()


@app.get("/", tags=["Root"])  # type: ignore
async def read_root(action: str | None = None) -> dict:
    num = int(os.getenv("NUM", "0"))
    # get param from link
    # lets say that I get a request like http://server:8000/?action=down
    if action == "down":
        raise HTTPException(status_code=500, detail=f"server {num} down due to maintenance")
    return {"message": f"welcome from server {num}"}

