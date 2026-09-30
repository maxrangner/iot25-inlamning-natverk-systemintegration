import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI

from . import config
from .mqtt_client import create_client
from .store import Store

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

