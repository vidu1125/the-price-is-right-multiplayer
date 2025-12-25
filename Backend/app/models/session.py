from app.models import db
from .base import Base
from sqlalchemy.sql import func
import uuid

class Session(Base):
    __tablename__ = "sessions"

    account_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        primary_key=True
    )

    session_id = db.Column(
        db.String(36),               # 👈 UUID portable
        nullable=False,
        unique=True,
        default=lambda: str(uuid.uuid4())
    )

    connected = db.Column(db.Boolean, default=True)

    updated_at = db.Column(
        db.DateTime,
        server_default=func.now(),
        onupdate=func.now()
    )
