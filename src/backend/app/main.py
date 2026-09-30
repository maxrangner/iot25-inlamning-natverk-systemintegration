import logging
import time
from contextlib import asynccontextmanager

from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from starlette.exceptions import HTTPException as StarletteHTTPException

from . import config
from .mqtt_client import create_client
from .store import Store

ERROR_CODES = {404: "not_found", 405: "method_not_allowed"}

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger(__name__)

store = Store()


@asynccontextmanager
async def lifespan(app):
    client = create_client(store)
    client.connect_async(config.MQTT_HOST, config.MQTT_PORT)
    client.loop_start()
    log.info("event=backend_started mqtt_host=%s mqtt_port=%s", config.MQTT_HOST, config.MQTT_PORT)
    yield
    client.disconnect()
    client.loop_stop()
    log.info("event=backend_stopped")


app = FastAPI(lifespan=lifespan)


@app.middleware("http")
async def log_request(request, call_next):
    start = time.monotonic()
    response = await call_next(request)
    duration_ms = round((time.monotonic() - start) * 1000)
    log.info(
        "event=api_request method=%s path=%s status=%s duration_ms=%s",
        request.method, request.url.path, response.status_code, duration_ms,
    )
    return response


@app.exception_handler(StarletteHTTPException)
async def http_error(request, error):
    return JSONResponse(
        status_code=error.status_code,
        content={"error": ERROR_CODES.get(error.status_code, "http_error"), "detail": error.detail},
    )


@app.get("/health")
def health():
    return {"status": "ok"}


@app.get("/api/readings/latest")
def latest_readings():
    readings = store.latest()
    if not readings:
        raise HTTPException(status_code=404, detail="No readings received yet")
    return readings


@app.get("/api/readings/{sensor_id}")
def reading(sensor_id: str):
    reading = store.get(sensor_id)
    if reading is None:
        raise HTTPException(status_code=404, detail=f"Unknown sensor {sensor_id}")
    return reading


@app.get("/api/status")
def status():
    return store.status()
